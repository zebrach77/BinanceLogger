#include "websocket.cpp"



int main(int argc, char** argv)
{
	// Инициализация переменных для работы с Binance
	auto host = "stream.binance.com";
	auto port = "9443";
	auto target = "/ws/btcusdt@bookTicker";
	auto text = "";

	net::io_context ioc;
	ssl::context ctx{ ssl::context::tlsv12_client };
	ctx.set_verify_mode(ssl::verify_none);
	load_root_certificates(ctx);

	// Запуск I/O сервиса
	ioc.run();

	//Переменная для ввода с консоли
	std::string inputStr;

	// Thread Pool для работы с потоками
	boost::asio::thread_pool pool(4);

	// Индикаторы начала и конца работы программы
	std::atomic<bool> isEnd = false;
	std::atomic<bool> isFirst = true;

	// Открываем файл для логов
	std::ofstream out;
	out.open("data_log.txt");



	// Инициализируем WebSocketSession
	std::shared_ptr<WebSocketSession> websocketSession;
	websocketSession = std::make_shared<WebSocketSession>(ioc, ctx, host, port, target);

	// Кладём в Pool лямбда-функцию для обработки событий и считывания с консоли команд
	boost::asio::post(pool,
			[&inputStr, &isEnd, &isFirst]() {
				while (inputStr[0] != 's' && inputStr[0] != EOF) {
					if (isFirst) {
						std::cout << "Press 'r' to run. Press 's' to stop." << std::endl;
					}
					std::cin >> inputStr;
					if (inputStr[0] == 'r') { //run
						isEnd = false;
						isFirst = false;
					}
				}
				if (inputStr[0] == 's') { //stop
					isEnd = true;
				}
			});

	// Кладём в Pool лямбда-функцию для работы с websocket
	boost::asio::post(pool,
			[&websocketSession, &ioc, &ctx, host, port, target, &isEnd, &isFirst]() {
				while (isEnd || isFirst){}
				while (!isEnd && !isFirst) {
					isFirst = false;
					websocketSession = std::make_shared<WebSocketSession>(ioc, ctx, host, port,
					                                                      target);
					websocketSession->Start();
				}

			});

	// Кладём в Pool лямбда-функцию для остановки
	boost::asio::post(pool,
			[&websocketSession, &isEnd, &isFirst]() {
				while (isEnd || isFirst){}
				while (!isEnd && !isFirst) {}
				if (isEnd) {
					websocketSession->Stop();
					std::exit(0);
				}
			});

	// Кладём в Pool лямбда-функцию для получения нужных данных и их вывода в логи
	boost::asio::post(pool,
			[&isEnd, &isFirst, &out, &websocketSession] {
		while (isEnd || isFirst){}
		while (!isEnd && !isFirst) {
			double temp_b = websocketSession->_bid_price;
			double temp_a = websocketSession->_ask_price;
			double temp_B = websocketSession->_bid_qty;
			double temp_A = websocketSession->_ask_qty;
			if (temp_a && temp_b) {
				PrintTimeToStream(out);
				out <<std::setprecision(10) << "(Bid + Oﬀer(Ask)) / 2 price: " << round_to_5((temp_b + temp_a)/2) <<",       "
				<< "(Bid + Oﬀer(Ask)) / 2 qty: " << round_to_5((temp_B + temp_A)/2)
				<< std::endl;
			}
			sleep(30);
		}
	});

	//Объединяем потоки
	pool.join();

	// Закрываем файл лога
	out.close();


	return EXIT_SUCCESS;
}