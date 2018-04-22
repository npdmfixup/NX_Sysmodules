#include <switch.h>
#include "sm_manager_service.hpp"

Result ManagerService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((ManagerServiceCmd)cmd_id) {
        case Manager_Cmd_RegisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::register_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Manager_Cmd_UnregisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::unregister_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}


std::tuple<Result> ManagerService::register_process(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac) {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> ManagerService::unregister_process(u64 pid) {
    /* TODO */
    return std::make_tuple(0xF601);
}