#include "./root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <string>
#include <boost/json.hpp>
#include <fstream>
#include <boost/asio/thread_pool.hpp>
#include <atomic>
#include <chrono>
#include <ctime>
#include <stdlib.h>
#include "colormod.h"
#include <cmath>
#include <iomanip>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <iostream>
#include <memory>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace json = boost::json;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// Округление до 2 знаков после запятой
double round_to_2(double value) {
	return round(value*100)/100;
}

// Округление до 5 знаков после запятой
double round_to_5(double value) {
	return round(value*100000)/100000;
}

// Печаать времени в предающийся поток с точностью до наносекунд
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
	out_stream << (hours.count()+11)%24<<":"<<minutes.count()<<":"<<seconds.count()<<"."<<nanoseconds.count()<<"          ";
}
