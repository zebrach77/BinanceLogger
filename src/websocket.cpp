#include "main.h"
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{

	std::ofstream _out; // Поток для печати в файл data_log.txt

	// Переменные для подключения к websocket
	net::io_context & _ioContext;
	ssl::context& _sslContext;
	tcp::resolver _tcpResolver;
	std::shared_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> _ws; // Поток для обмена данными с сервером
	beast::flat_buffer _buffer; // Буфер для временного хранения полученных данных
	std::string _host;
	std::string _hostAndPort;
	char const * _port;
	std::string _target;
	std::string _text;
	tcp::endpoint _tcpEndPoint;
	bool _terminate = false; // Индикатор конца сессии


	void ConnectAndReceive()
	{

		// Поиск доменного имени
		auto const tcpResolverResults = _tcpResolver.resolve(_host, _port);

		// Подключение к полученному IP-адресу
		_tcpEndPoint = net::connect(get_lowest_layer(*_ws), tcpResolverResults);

		// Установка SNI Hostname
		if (!SSL_set_tlsext_host_name(_ws->next_layer().native_handle(), _host.c_str())) {
			throw beast::system_error(
					beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "Failed to set SNI Hostname");
		}
		// Генерация полноценного имени хоста
		_hostAndPort = _host + ':' + std::to_string(_tcpEndPoint.port());

		// SSL рукопожатие
		_ws->next_layer().handshake(ssl::stream_base::client);

		// Декоратор для замены User-agent в протоколе рукопожатия
		_ws->set_option(websocket::stream_base::decorator(
				[](websocket::request_type& req)
				{
					req.set(http::field::user_agent,
					        std::string(BOOST_BEAST_VERSION_STRING) +
					        " websocket-client-coro");
				}));

		// Websocket рукопожатие
		_ws->handshake(_hostAndPort, _target);

		//Чтение сообщений
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
				PrintTimeToStream(_out);
				_out << "socket was closed." << std::endl;
			}
			else {
				PrintTimeToStream(_out);
				_out << "exception: " << se.code() << ", " << se.code().message() << ", " << se.what();
			}
		}
		catch (std::exception& ex) {
			PrintTimeToStream(_out);
			_out << "exception: " << ex.what();
		}

	}
	// Метод для сохранения и печати нужной информации из буфера
	void PrintAndStoreJSON(auto b) {
		// Получение строки из буфера
		std::string s(net::buffer_cast<const char*>(b.data()), b.size());

		//Получение json-объекта из строки
		json::object jv = json::value_to<json::object>(json::parse(s));

		// Получение нужных данных из json-объекта
		_bid_price = std::stof(json::value_to<std::string>(jv["b"]));
		_ask_price = std::stof(json::value_to<std::string>(jv["a"]));
		_bid_qty = std::stof(json::value_to<std::string>(jv["B"]));
		_ask_qty = std::stof(json::value_to<std::string>(jv["A"]));

		//Настройка цветовых акцентов (См. colormod.h)
		Color::Modifier green(Color::FG_GREEN);
		Color::Modifier def(Color::FG_DEFAULT);

		// Очистка консоли перед выводом
		system("clear");

		// Изменение максимальной длины выводимого числа в cout
		std::cout.precision(10);

		// Вывод в консоль
		std::cout <<"------------------------------------------------"<<std::endl
	      << "Bid price: "
	      <<green<< round_to_2(_bid_price) <<def<< std::endl<< "Offer(Ask) price: "
	      <<green<< round_to_2(_ask_price)<< def<< std::endl <<"Bid qty: "
	      <<green<< round_to_5(_bid_qty)<< def<< std::endl <<"Offer(Ask) qty: "
	      <<green<< round_to_5(_ask_qty)<< def<< std::endl <<
		  std::endl<<"------------------------------------------------"<<std::endl;

		// Вывод информации в data_log.txt
		PrintTimeToStream(_out);
		_out<<"Connected, data received." << std::endl;
	}


public:
	// Public переменные для передачи в main.cpp
	double _ask_price = 0.0;
	double _bid_price = 0.0;
	double _ask_qty = 0.0;
	double _bid_qty = 0.0;

	// Конструктор
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

	// Остановка работы
	void Stop() {
		_terminate = true;
		_out.close();
	}

	// Начало работы
	void Start()
	{
		while (!_terminate) {

			_ws = std::make_shared<websocket::stream<beast::ssl_stream<tcp::socket>>>(_ioContext, _sslContext);

			try {
				ConnectAndReceive(); //Инициализация websocket и запрос сообщений, пока подключение не закрыто
			}
			catch (beast::system_error const& se) {
				PrintTimeToStream(_out);
				_out << "exception: " << se.code() << ", " << se.code().message() << std::endl;
				sleep(1000); //Ждём, чтобы не спамить
			}

			_ws.reset();
		}
	}

};