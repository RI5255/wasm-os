#include <libs/common/message.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>

int main(void) {
    // Find {tcpip, echo} server.
    task_t tcpip_server = ipc_lookup("tcpip");
    task_t echo_server = ipc_lookup("echo");

    // Listen on port 80.
    struct message m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.listen_port = 80;
    ASSERT_OK(ipc_call(tcpip_server, &m));

    while(true) {
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        DBG(
            "received message from #%d: %s (%x)", m.src,
            msgtype2str(m.type),
            m.type
        );
        
        switch(m.type) {
            case TCPIP_CLOSED_MSG: {
                // Destroy socket.
                m.type = TCPIP_DESTROY_MSG;
                m.tcpip_destroy.sock = m.tcpip_closed.sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));
                break;
            }
            case TCPIP_DATA_MSG: {
                int sock = m.tcpip_data.sock;

                // Read the received data.
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));

                char data[1024];
                size_t data_len = m.tcpip_read_reply.data_len;
                memcpy(data, m.tcpip_read_reply.data, data_len);

                DBG("received data (%u bytes) from socket %d", data_len, sock);

                // Relay to the echo server.
                m.type = ECHO_MSG;
                m.echo.data_len = data_len;
                memcpy(m.echo.data, data, data_len);
                ASSERT_OK(ipc_call(echo_server, &m));
                ASSERT(m.echo_reply.data_len == data_len);
                
                // Replay to the client.
                memcpy(data, m.echo_reply.data, data_len);
                m.type = TCPIP_WRITE_MSG;
                m.tcpip_write.sock = sock;
                memcpy(m.tcpip_write.data, data, data_len);
                m.tcpip_write.data_len = data_len;
                ASSERT_OK(ipc_call(tcpip_server, &m));
                break;
            }
        }
    }
}