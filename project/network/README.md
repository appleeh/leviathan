# leviathan

network : Framework for building servers very quickly

        verification completed : iocp/redis
        
        todo : 
        
          - Provides a more organized interface
          - provide epoll-based framework
          - provide buffer according to http contents size analysis



<<<<<<< Updated upstream
![network-1](.\img\network-1.PNG)


=======
![](https://github.com/appleeh/leviathan/blob/master/project/network/img/network-1.PNG)
>>>>>>> Stashed changes



# IOCP 구조

1. Iocp 는 하나의 iocp 큐를 가지고 여러 소켓처리를 하도록 구성할수 있다.
2. Iocp 큐에 들어온 작업들은 OS 에서 자동으로 최적화된 쓰레드를 지정하여 매칭시킨다.
3. 모든 소켓들을 하나의 IOCP 에 지정해 놓으면 나머지는 알아서 처리된다.



#### 장점

1. 다중 소켓 처리를 특정 쓰레드 풀에서만 처리하여
    지나친 쓰레드 생성 및 컨택스트 스위칭을 줄일수 있다.
2. OS 가 가장 최적화된 쓰레드 매칭을 하도록 설계되어 있다. 
    (즉 가장 최근에 썼던 쓰레드를 계속 쓰는 경향)
3. 다중 쓰레드지만, 현재 처리하고 있는 소켓의 Recv 처리가
    다중으로 처리 되지 않는다. 처리 완료후 bindRecv() 되기 때문.
4. 동일 소켓의 Recv 와 Send 는 다중 쓰레드에 의해 동시에 실행 가능.
5. Send 는 Overlapped 지원 소켓에 의해 다중 쓰레드로 처리 가능하나, 하나의 소켓에 대한 부하가 클경우
   경우에 따라 queue - thread 를 하나더 추가 하기를 권장



#### 처리 매커니즘

1. Comunicator

   - 두 개의 함수 포인터를 등록하여 처리 
     - 길이 parsing 하여 패킷 길이 만큼 버퍼에 담아주는 함수 포인터
     - 해당 패킷을 인자로 넘겨 처리해주는 함수포인터

   - 하나의 IOCP 와 ThreadPool 을 가짐
   - 클라이언트를 관리할수 있는 Connection List 를 필요시 가짐
   - IOCP 와 연동되는 주요 로직을 정의

2. ComunicatorList
   - 서로 다른 프로토콜을 다룰때, 다중 Comunicator 를 생성 할 수 있음

3. SessionManager
   - 메모리 Pool 개념 ; 싱글톤과 같이 하나만 존재하여 IOCP 전체에서 공유하는 자원 
     - 소켓 객체 메모리 POOL
     - Overlapped 를 지원하는 자료구조와 버퍼 POOL



![network-2](.\img\network-2.PNG)

#### 소켓 버퍼 전략

1. RecvBuf , SendBuf 두개를 운영한다.

2. IOCP 와 연동하는 소켓에 할당하는 recv 버퍼 사이즈는 패킷 길이에 따라 가변적으로 할당한다.

3. RecvBuf : 가변적 할당은 더 큰 사이즈로 할당이 필요할 때만 변경된다. (기존 사이즈 버퍼 반환)

4. SendBuf : Send 완료후 상시 반환. Overlapped 를 지원함

   

#### 헤더파싱

1. 숫자 길이 파싱
     \- 숫자 byte 가변적 길이 INI 파일에서 읽어서 적용
2. Html 헤더 파싱 (이후 처리)
     \- CONTENTS_LENTH, Chunk
3. 길이가 문자열 일때, 문자열 길이 헤더 파싱
     \- 뒤에 끝을 알리는 문자 INI 파일에서 읽어서 적용

   ※ 길이 Parsing 이후, 기존에 할당된 RecvBuf 사이즈가 부족하면, 반환하고 다시 할당한다. 



#### 다중 IOCP 구조 활용

1. IOCP 큐 하나를 소켓 처리가 아닌 순수 이벤트 처리 쓰레드로 활용 가능하다. 
   이때는 SessionManager 를 사용안함으로 지정해야 한다.
2. IOCP 큐를 작업별로 분할하여 역할을 쪼갤수 있다.



![network-4](.\img\network-4.PNG)







# epoll 구조

1. Epoll 구조는 소켓 하나당 또는 특정 이벤트처리에 대해 초기에 명확하게 epoll 을 지정해야 한다.
2. Epoll 은 처음에 epoll 을 지정하는 로직이 추가로 들어간다.
3. Epoll 은 Epoll 하나당 쓰레드 하나로 지정된다. 그렇기에 쓰레드Pool 에는 쓰레드 개수만큼 epoll 이 생성된다.



###### 장점

1. 하나의 소켓은 하나의 지정된 쓰레드에서 처리된다.
2. 다중 소켓 처리를 특정 쓰레드 풀에서만 처리하여
    지나친 쓰레드 생성 및 컨택스트 스위칭을 줄일수 있다.



###### 처리 매커니즘

1. IOCP 와 전반적으로 구조는 동일.
2. 단 소켓 생성시 특정 epoll 을 할당
   - ServerSocket 은 생성 초기시에 특정 epoll 을 할당
   - ListenSocket 은 생성 초기시에 특정 epoll 을 할당
   - ClientSocket 은 accept 시에 특정 epoll 을 할당



![network-3](.\img\network-3.PNG)





