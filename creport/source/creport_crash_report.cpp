#include <switch.h>
#include "creport_crash_report.hpp"
#include "creport_debug_types.hpp"

void CrashReport::SaveReport() {
    /* TODO: Save the report to the SD card. */
}

void CrashReport::BuildReport(u64 pid, bool has_extra_info) {
    this->has_extra_info = has_extra_info;
    if (OpenProcess(pid)) {
        ProcessExceptions();
        
        /* TODO: More stuff here (sub_7100002260)... */
        Close();
    }
}

void CrashReport::ProcessExceptions() {
    if (!IsOpen()) {
        return;
    }
    
    DebugEventInfo d;
    while (R_SUCCEEDED(svcGetDebugEvent((u8 *)&d, this->debug_handle))) {
        switch (d.type) {
            case DebugEventType::AttachProcess:
                HandleAttachProcess(d);
                break;
            case DebugEventType::Exception:
                HandleException(d);
                break;
            case DebugEventType::AttachThread:
            case DebugEventType::ExitProcess:
            case DebugEventType::ExitThread:
            default:
                break;
        }
    }
}

void CrashReport::HandleAttachProcess(DebugEventInfo &d) {
    this->process_info = d.info.attach_process;
    if (kernelAbove500() && IsApplication()) {
        /* Parse out user data. */
        u64 address = this->process_info.user_exception_context_address;
        u64 userdata_address = 0;
        u64 userdata_size = 0;
        
        if (!IsAddressReadable(address, sizeof(userdata_address) + sizeof(userdata_size))) {
            return;
        }
        
        /* Read userdata address. */
        if (R_FAILED(svcReadDebugProcessMemory(&userdata_address, this->debug_handle, address, sizeof(userdata_address)))) {
            return;
        }
        
        /* Validate userdata address. */
        if (userdata_address == 0 || userdata_address & 0xFFF) {
            return;
        }
        
        /* Read userdata size. */
        if (R_FAILED(svcReadDebugProcessMemory(&userdata_size, this->debug_handle, address + sizeof(userdata_address), sizeof(userdata_size)))) {
            return;
        }
        
        /* Cap userdata size. */
        if (userdata_size > 0x1000) {
            userdata_size = 0x1000;
        }
        
        /* Assign. */
        this->userdata_5x_address = userdata_address;
        this->userdata_5x_size = userdata_size;
    }
}

void CrashReport::HandleException(DebugEventInfo &d) {
    this->exception_info = d.info.exception;
    switch (d.info.exception.type) {
        case DebugExceptionType::UndefinedInstruction:
            this->result = (Result)CrashReportResult::UndefinedInstruction;
            break;
        case DebugExceptionType::InstructionAbort:
            this->result = (Result)CrashReportResult::InstructionAbort;
            this->exception_info.specific.raw = 0;
            break;
        case DebugExceptionType::DataAbort:
            this->result = (Result)CrashReportResult::DataAbort;
            break;
        case DebugExceptionType::AlignmentFault:
            this->result = (Result)CrashReportResult::AlignmentFault;
            break;
        case DebugExceptionType::UserBreak:
            this->result = (Result)CrashReportResult::UserBreak;
            /* Try to parse out the user break result. */
            if (kernelAbove500() && IsAddressReadable(this->exception_info.specific.user_break.address, sizeof(this->result))) {
                svcReadDebugProcessMemory(&this->result, this->debug_handle, this->exception_info.specific.user_break.address, sizeof(this->result));
            }
            break;
        case DebugExceptionType::BadSvc:
            this->result = (Result)CrashReportResult::BadSvc;
            break;
        case DebugExceptionType::UnknownNine:
            this->result = (Result)CrashReportResult::UnknownNine;
            this->exception_info.specific.raw = 0;
            break;
        case DebugExceptionType::DebuggerAttached:
        case DebugExceptionType::BreakPoint:
        case DebugExceptionType::DebuggerBreak:
        default:
            return;
    }
    /* TODO: Parse crashing thread info. */
}

bool CrashReport::IsAddressReadable(u64 address, u64 size, MemoryInfo *o_mi) {
    MemoryInfo mi;
    u32 pi;
    
    if (o_mi == NULL) {
        o_mi = &mi;
    }
    
    if (R_FAILED(svcQueryDebugProcessMemory(o_mi, &pi, this->debug_handle, address))) {
        return false;
    }
    
    /* Must be read or read-write */
    if ((o_mi->perm | Perm_W) != Perm_Rw) {
        return false;
    }
    
    /* Must have space for both userdata address and userdata size. */
    if (address < o_mi->addr || o_mi->addr + o_mi->size < address + size) {
        return false;
    }

    return true;
}