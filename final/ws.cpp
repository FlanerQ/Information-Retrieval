#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// 声明 WebSocket 服务器类型
typedef websocketpp::server<websocketpp::config::asio> server;

void func(std::string a){
    std::cout<<"func: "<<a<<std::endl;
}
void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::string query = msg->get_payload();
    // 发送响应消息
    s->send(hdl, query, msg->get_opcode());
}

int main() {
    // 创建 WebSocket 服务器对象
    server ws_server;
    ws_server.init_asio();

    // 设置 WebSocket 回调函数，当收到消息时执行
    ws_server.set_message_handler(std::bind(&on_message, &ws_server, std::placeholders::_1, std::placeholders::_2));

    // 监听端口，并启动 WebSocket 服务器
    ws_server.listen(8080);
    ws_server.start_accept();
    std::cout<<"Server start"<<std::endl;

    // 进入事件循环
    ws_server.run();
}
