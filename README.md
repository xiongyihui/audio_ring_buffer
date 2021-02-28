Realtime Audio Ring Buffer
==========================

Single Producer, Multiple Consumers

1. 只记录写指针，只有一个生产者，可以有多个消费者，消费者自己维护读指针
2. 有新数据时，广播消息
3. 消费者长时间没有读数据时，读指针指向的数据已经被新数据覆盖，消费者自己更新读指针
4. 没有数据可读时，等待新数据广播
5. 避免某一块buffer被同时读写，ring buffer的可读长度可以小于ring buffer的长度


