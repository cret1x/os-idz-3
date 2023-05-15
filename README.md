# ИДЗ №3
## Амирханов Никита БПИ219
## Вариант 1
Задача о парикмахере. В тихом городке есть парикмахерская. Салон парикмахерской мал, работать в нем нем может только один парикмахер, обслуживающий одного посетителя. Есть несколько кресел для ожидания в очереди. Парикмахер всю жизнь обслуживает посетителей. Когда в салоне никого нет, он спит в кресле. Когда посетитель приходит и видит спящего парикмахера, он будет его, садится в кресло, «засыпая» на тот момент, пока парикмахер обслуживает его. Если посетитель приходит, а парикмахер занят, то он встает в очередь и «засыпает». После стрижки парикмахер сам провожает посетителя. Если есть ожидающие посетители, то парикмахер будит одного из них и ждет пока тот сядет в кресло парикмахера и начинает стрижку. Если никого нет, он снова садится в свое кресло и засыпает до прихода посетителя. Создать приложение, моделирующее рабочий день парикмахерской.

### Запуск кода
В задании на оценку 4-5:
```sh
gcc client.c -o client
gcc server.c -o server -lrt -lpthread
./server <port>
./client <server ip> <server port>
```
В заданиях 6-10:
```sh
gcc client.c -o client
gcc listener.c -o listener
gcc server.c -o server -lrt -lpthread
./server <port for clients> <port for listeners>
./listener <server ip> <server port>
./client <server ip> <server port>
```


### 4-5
Имеется приложение, создается сервер на заданном порту. К нему подключаются клиенты. Максимальное число клиентов в очереди - 10. Когда сервер может принять клиента, он принимает от него его id, и отправляет ему сообщение чтобы клиент ждал. После чего он работает с клиентом, и отправляет сообщение что закончил. После этого закрывает сокет клиента и становится готов принять нового.
Пример работы:
```sh
./server 8001
[SYSTEM] Service 'Cutter' is running on 0.0.0.0:8001
[SYSTEM] New connection from 127.0.0.1
[Cutter] Got new client
[Cutter] Working on client #5965
[Cutter] Finished client #5965
[SYSTEM] New connection from 127.0.0.1
[Cutter] Got new client
[Cutter] Working on client #5966
[Cutter] Finished client #5966
```
```sh
./client 127.0.0.1 8001
I am client #5965, want to got to cutter
Getting hair cut
Finished
./client 127.0.0.1 8001
I am client #5966, want to got to cutter
Getting hair cut
Finished

```
### 6-10
К прошлому приложению добавляется возможность подключения наблюдаделя к серверу. Наблюдатель может подключаться и отключаться в любой момент. Наблюдателей может быть несколько. Работа с клиентами происходит так же как и в задании на прошлую оценку. При отключении сервера, все клиенты и наблюдатели завершают работу. При отключении клиента, сервер продолжает работу
Пример работы:
```sh
./server 8000 8001
[SYSTEM] Service 'Cutter' is running on 0.0.0.0:8000
[SYSTEM] Service 'Notifier' is running on 0.0.0.0:8001
[SYSTEM] New connection from 127.0.0.1
[SYSTEM] Listener connected
[SYSTEM] New connection from 127.0.0.1
[Cutter] Got new client
[Cutter] Working on client #6014
[Cutter] Finished client #6014
[SYSTEM] Listener disconnected
[SYSTEM] New connection from 127.0.0.1
[SYSTEM] Listener connected
[SYSTEM] New connection from 127.0.0.1
[Cutter] Got new client
[Cutter] Working on client #6017
[Cutter] Finished client #6017
[SYSTEM] Listener disconnected
```

```sh
./client 127.0.0.1 8000
I am client #6014, want to got to cutter
Getting hair cut
Finished
./client 127.0.0.1 8000
I am client #6017, want to got to cutter
Getting hair cut
Finished

```

```sh
./listener 127.0.0.1 8001
I am Listener process
Connected to server
Current client id: -1
Current client id: -1
Current client id: 6014
Current client id: 6014
Current client id: 6014
Current client id: -1
Current client id: -1

./listener 127.0.0.1 8001
I am Listener process
Connected to server
Current client id: -1
Current client id: -1
Current client id: -1
Current client id: 6017
Current client id: 6017
Current client id: -1
```