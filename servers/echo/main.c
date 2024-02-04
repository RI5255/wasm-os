#include <libs/common/print.h>
#include <libs/common/message.h>
#include <libs/user/ipc.h>

int main(void) {
    // Register as echo server.
    ASSERT_OK(ipc_register("echo"));
    TRACE("ready");

    while (true) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case ECHO_MSG: {
                DBG("received echo message from #%d (len = %d)", m.src, m.echo.data_len);
                m.type = ECHO_REPLY_MSG;
                // Assume that echo and echo_reply message have the same layout.
                ipc_reply(m.src, &m);
                break;
            }
            default:
                WARN("unhandled message: %s (%x)", msgtype2str(m.type), m.type);
                break;
        }
    }
}