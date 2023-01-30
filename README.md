## BinanceLogger
### Инструкция по установке
[1] Установка необходимых библиотек
```
sudo apt-get update
sudo apt-get install git
sudo apt-get install libboost-dev
sudo apt-get install libssl-dev
```


[2.1] Сборка проекта, если он ещё не скачан, при помощи git
```
git clone https://github.com/zebrach77/BinanceLogger.git
cd BinanceLogger
cmake .
make
```

[2.2] Сборка проекта, если он скачан в zip архиве
```
sudo apt-get install unzip
mkdir BinanceLogger
unzip {имя архива} -d BinanceLogger
cd BinanceLogger
cmake .
make
```

[3] Запуск
```
./BinanceLogger
```


[4] Удаление всех созданных объектных файлов
```
sudo chmod -R 777 delete_script
./delete_script
```

Логи подключения пишутся в connection_log.txt


Логи данных пишутся в data_log.txt

