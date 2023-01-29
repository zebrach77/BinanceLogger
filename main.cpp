#include "./root_certificates.hpp"
#include <thread>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <string>
#include <mutex>
#include <condition_variable>
#include <boost/json.hpp>
#include <fstream>
#include "thread_pool.cpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <stdlib.h>
#include "colormod.h"
#include <cmath>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace json = boost::json;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>



double round_to_2(double value) {
	return round(value*100)/100;
}
double round_to_5(double value) {
	return round(value*100000)/100000;
}


void PrintTimeToStream(std::ofstream &out_stream){
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	time_t tt = std::chrono::system_clock::to_time_t(now);
	tm local_tm = *localtime(&tt);


	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<8>>::type> Days; /* UTC: +8:00 */
	Days days = std::chrono::duration_cast<Days>(duration);
	duration -= days;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

	out_stream <<local_tm.tm_mon+1<<"/"<<local_tm.tm_mday<<"/"<<local_tm.tm_year+1900<<" ";
	out_stream << hours.count()+19<<":"<<minutes.count()<<":"<<seconds.count()<<"."<<nanoseconds.count()<<"          ";
}


//Class to connect to a websocket endpoint and receive data continuously counterparty closes or stopped manually
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
	std::ofstream _out;
	net::io_context & _ioContext;
	ssl::context& _sslContext;
	tcp::resolver _tcpResolver;
	std::shared_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> _ws;
	beast::flat_buffer _buffer;
	std::string _host;
	std::string _hostAndPort;
	char const * _port;
	std::string _target;
	std::string _text;
	tcp::endpoint _tcpEndPoint;
	bool _terminate = false;


	void ConnectAndReceive()
	{

		// Look up the domain name
		auto const tcpResolverResults = _tcpResolver.resolve(_host, _port);

		// Make the connection on the IP address we get from a lookup
		_tcpEndPoint = net::connect(get_lowest_layer(*_ws), tcpResolverResults);

		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(_ws->next_layer().native_handle(), _host.c_str())) {
			throw beast::system_error(
					beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
		}
		// Update the _host string. This will provide the value of the
		// Host HTTP header during the WebSocket handshake.
		// See https://tools.ietf.org/html/rfc7230#section-5.4
		_hostAndPort = _host + ':' + std::to_string(_tcpEndPoint.port());

		// Perform the SSL handshake
		_ws->next_layer().handshake(ssl::stream_base::client);

		// Set a decorator to change the User-Agent of the handshake
		_ws->set_option(websocket::stream_base::decorator(
				[](websocket::request_type& req)
				{
					req.set(http::field::user_agent,
					        std::string(BOOST_BEAST_VERSION_STRING) +
					        " websocket-client-coro");
				}));

		// Perform the websocket handshake
		_ws->handshake(_hostAndPort, _target);

		//Read Messages
		try {
			while (true) //expect to read continuously until a connection failure
			{
				if (!_terminate) {
					_ws->read(_buffer);

					if (_buffer.size() > 0)
					{
						PrintAndStoreJSON(_buffer);
					}
					_buffer.clear();
				}
				else {
					_ws->close(boost::beast::websocket::close_code::normal);
				}

			}
		}
		catch (beast::system_error const& se) {
			if (se.code() == websocket::error::closed) {
				PrintTime();
				_out << "socket was closed." << std::endl;
			}
			else {
				PrintTime();
				_out << "exception: " << se.code() << ", " << se.code().message() << ", " << se.what();
			}
		}
		catch (std::exception& ex) {
			PrintTime();
			_out << "exception: " << ex.what();
		}

	}
	void PrintAndStoreJSON(auto b) {
		std::string s(net::buffer_cast<const char*>(b.data()), b.size());
		json::object jv = json::value_to<json::object>(json::parse(s));
//		std::string temp = json::value_to<std::string>(jv["b"]);
		_bid_price = std::stof(json::value_to<std::string>(jv["b"]));
		_ask_price = std::stof(json::value_to<std::string>(jv["a"]));
		_bid_qty = std::stof(json::value_to<std::string>(jv["B"]));
		_ask_qty = std::stof(json::value_to<std::string>(jv["A"]));
		Color::Modifier green(Color::FG_GREEN);
		Color::Modifier def(Color::FG_DEFAULT);
		system("clear");
		std::cout.precision(10);
		std::cout <<"------------------------------------------------"<<std::endl
		<< "Bid price: "
		<<green<< round_to_2(_bid_price) <<def<< std::endl<< "Offer(Ask) price: "
		<<green<< round_to_2(_ask_price)<< def<< std::endl <<"Bid qty: "
		<<green<< round_to_5(_bid_qty)<< def<< std::endl <<"Offer(Ask) qty: "
		<<green<< round_to_5(_ask_qty)<< def<< std::endl << std::endl<<"------------------------------------------------"<<std::endl;
		PrintTime();
		_out<<"Connected, data received." << std::endl;
		return;
	}

	void PrintTime(){
		PrintTimeToStream(_out);
	}

public:
	double _ask_price = 0.0;
	double _bid_price = 0.0;
	double _ask_qty = 0.0;
	double _bid_qty = 0.0;
	// Resolver and socket require an io_context
	explicit
	WebSocketSession(net::io_context& ioc,
	                 ssl::context& ctx,
	                 char const* host,
	                 char const* port,
	                 char const* target)
			: _ioContext(ioc),
			  _sslContext(ctx),
			  _tcpResolver(_ioContext),
			  _host(host),
			  _port(port),
			  _target(target),
			  _hostAndPort()
	{
		_out.open("connection_log.txt");
	}

	void Stop() {
		_terminate = true;
		_out.close();
	}

	void Start()
	{
		while (!_terminate) {

			_ws = std::make_shared<websocket::stream<beast::ssl_stream<tcp::socket>>>(_ioContext, _sslContext);

			try {
				ConnectAndReceive(); //initalizes websocket and polls continuously for messages until socket closed or an exception occurs
			}
			catch (beast::system_error const& se) {
				PrintTime();
				_out << "exception: " << se.code() << ", " << se.code().message() << std::endl;
				sleep(1000); //just wait a little while to not spam
			}

			_ws.reset();
		}

		//if we get to here process has been terminated
	}

};

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
	auto host = "stream.binance.com";
	auto port = "9443";
	auto target = "/ws/btcusdt@bookTicker"; //bitcoin full order book endpoint, 100ms updates
	auto text = "";

	// The io_context is required for all I/O
	net::io_context ioc;
	// The SSL context is required, and holds certificates
	ssl::context ctx{ ssl::context::tlsv12_client };
	ctx.set_verify_mode(ssl::verify_none);
	// This holds the root certificate used for verification
	load_root_certificates(ctx);

	// Run the I/O service. The call will return when
	// the socket is closed.
	ioc.run();

	std::string inputStr;
	tp::ThreadPool pool{4};
	std::atomic<bool> isEnd = false;
	std::atomic<bool> isFirst = true;
	std::shared_ptr<WebSocketSession> websocketSession;
	std::ofstream out;
	out.open("data_log.txt");
	double temp_b;
	double temp_a;
	double temp_B;
	double temp_A;
	std::time_t now;



	websocketSession = std::make_shared<WebSocketSession>(ioc, ctx, host, port, target);
	pool.Submit(
			[&websocketSession, &ioc, &ctx, host, port, target, &temp_b, &temp_a, &inputStr, &isEnd, &isFirst]() {
				while (inputStr[0] != 'x' && inputStr[0] != 's' && inputStr[0] != EOF) {
					if (isFirst) {
						std::cout << "Press 'r' to run. Press 's' to stop. Press 'x' to Exit." << std::endl;
					}
					std::cin >> inputStr;
					if (inputStr[0] == 'r') { //run
						isEnd = false;
						isFirst = false;
					}
				}
				if (inputStr[0] == 's' || inputStr[0] == 'x') { //stop & exit
					isEnd = true;
				}
			});

	pool.Submit(
			[&websocketSession, &ioc, &ctx, host, port, target, &temp_b, &temp_a, &inputStr, &isEnd, &isFirst]() {
				while (isEnd || isFirst){}
				while (!isEnd && !isFirst) {
					isFirst = false;
					websocketSession = std::make_shared<WebSocketSession>(ioc, ctx, host, port,
					                                                      target);
					websocketSession->Start(); //within a thread so doesn't block
				}

			});
	pool.Submit(
			[&websocketSession, &ioc, &ctx, host, port, target, &temp_b, &temp_a, &inputStr, &isEnd, &isFirst]() {
				while (isEnd || isFirst){}
				while (!isEnd && !isFirst) {}
				if (isEnd) {
					websocketSession->Stop();
				}
			});

	pool.Submit([&temp_b, &temp_a, &temp_B, &temp_A, &inputStr, &isEnd, &isFirst, &out, &websocketSession, &now] {
		while (isEnd || isFirst){}
		while (!isEnd && !isFirst) {
			temp_b = websocketSession->_bid_price;
			temp_a = websocketSession->_ask_price;
			temp_B = websocketSession->_bid_qty;
			temp_A = websocketSession->_ask_qty;
			if (temp_a && temp_b && temp_A && temp_B) {
				PrintTimeToStream(out);
				out <<std::setprecision(10) << "(Bid + Oﬀer(Ask)) / 2 price: " << round_to_5((temp_b + temp_a)/2) <<",       "
				<< "(Bid + Oﬀer(Ask)) / 2 qty: " << round_to_5((temp_B + temp_A)/2)
				<< std::endl;
			}
			sleep(1);
		}
	});


	pool.Join();
	out.close();


	return EXIT_SUCCESS;
}