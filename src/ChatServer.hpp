#pragma once
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "MsgPool.hpp"
#include "LogSvr.hpp"
#include "ConnectInfo.hpp"
#include "UserManager.hpp"
#include "Message.hpp"

#define UDP_PORT 17777
#define TCP_PORT 17778
#define THREAD_COUNT 2

//当前类实现
//1.接受用户数据
//2.发送数据消息给客户端
//依赖UDP协议实现
class ChatServer
{
  public:
    ChatServer()
    {
      UdpSock_ = -1;
      UdpPort_ = UDP_PORT;
      MsgPool_ = NULL;
      TcpSock_ = -1;
      TcpPort_ = TCP_PORT;
      UserManager_ = NULL;
    }

    ~ChatServer()
    {
      if(MsgPool_)
      {
        delete MsgPool_;
        MsgPool_ = NULL;
      }

      if(UserManager_)
      {
        delete UserManager_;
        UserManager_ = NULL;
      }
    }

    //上次调用InitServer函数来初始化UDP服务
    void InitSerber()
    {
      //1.创建UDP套接字
      UdpSock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if(UdpSock_ < 0)
      {
        LOG(FATAL, "Create socket failed") << std::endl;
        exit(1);
      }
      //2.绑定地址信息 
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(UdpPort_);
      addr.sin_addr.s_addr = inet_addr("0.0.0.0");

      int ret = bind(UdpSock_, (struct sockaddr*)&addr, sizeof(addr));
      if(ret < 0)
      {
        LOG(FATAL,  "Bind addrinfo failed") << std::endl;
        exit(2);
      }

      LOG(INFO, "Udp bind success") << std::endl;
      //初始化数据池
      MsgPool_ = new MsgPool();
      if(!MsgPool_)
      {
        LOG(FATAL, "Create Msgpool failed")<<  std::endl;
        exit(3);
      }
      LOG(INFO, "Create MsgPool success") << std::endl;
  
      UserManager_ = new UserManager();
      if(!UserManager_)
      {
        LOG(FATAL, "Create User manager failed") << std::endl;
        exit(8);
      }

      //创建TCP-socket;
      TcpSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(TcpSock_ < 0)
      {
        LOG(FATAL, "Create tcp socket failed") << std::endl;
        exit(5);
      }
      int opt = 1;
      setsockopt(TcpSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

      struct sockaddr_in tcpaddr;
      tcpaddr.sin_family = AF_INET;
      tcpaddr.sin_port = htons(TcpPort_);
      tcpaddr.sin_addr.s_addr = inet_addr("0.0.0.0"); 
      bind(TcpSock_, (struct sockaddr*)&tcpaddr, sizeof(tcpaddr));
      if(ret < 0)
      {
        LOG(FATAL, "Bind Tcp addrinfo failed") << std::endl;
        exit(6);
      }

      ret = listen(TcpSock_, 5);
      if(ret < 0)
      {
        LOG(FATAL, "Tcp listen failed") << std::endl;
        exit(7);
      }
      LOG(INFO, "Tcp listen 0.0.0.0:17778") << std::endl;
    }

    //初始化程序当中生产和消费线程
    void Start()
    {
      pthread_t tid;
      for(int i = 0; i < THREAD_COUNT; i++)
      {
        int ret = pthread_create(&tid, NULL, ProductMsgStart, (void*)this);
        if (ret < 0)
        {
          LOG(FATAL, "pthread_creat new thread failed") << std::endl;
          exit(4);
        }

        ret = pthread_create(&tid, NULL, ConsumeMsgStart, (void*)this);
        if (ret < 0)                                  
        {                                             
          perror("pthread_creat new thread failed");
          
          exit(4);
        } 
      }
      LOG(INFO, "UdpChat Service start success") << std::endl;

      while(1)
      {
        struct sockaddr_in cliaddr;
        socklen_t cliaddrlen = sizeof(cliaddr);
        int newsock =  accept(TcpSock_, (struct sockaddr*)&cliaddr, &cliaddrlen);
        if(newsock < 0)
        {
          LOG(ERROR, "Accept new connect failed") << std::endl;
          continue;
        }
        LoginConnect* lc = new LoginConnect(newsock, (void*)this);
        if(!lc)
        {
          LOG(ERROR, "Create LoginConnect failed") << std::endl;
          continue;
        }
        //创建线程，处理登陆、注册的请求
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, LoginRegStart, (void*)lc);
        if(ret < 0)
        {
          LOG(ERROR, "Create User LoginConnect thread failed") << std::endl;
          continue;
        }
        LOG(INFO, "Create TcpConnect thread success") << std::endl;
      }

      LOG(INFO, "UdpChat Service start success") << std::endl;
    }
    

  private:
    static void* ProductMsgStart(void* arg)
    {
      pthread_detach(pthread_self());
      ChatServer* cs = (ChatServer*)arg;
      while(1)
      {
        //recvfrom
        cs->ReccMsg();
      }
      return NULL;
    }

    static void* ConsumeMsgStart(void* arg)
    {
      pthread_detach(pthread_self());
      ChatServer* cs = (ChatServer*)arg;
      while(1)
      {
        //sendto
        cs->BroadcastMsg();
      }
      return NULL;
    }

    static void* LoginRegStart(void* arg)
    {
      pthread_detach(pthread_self());
      LoginConnect* lc = (LoginConnect*)arg;
      ChatServer* cs = (ChatServer*)lc->GetServer();
      //注册，登陆请求
      
      //请求从cli端来，recv；
      char QuesType;
      ssize_t recvsize = recv(lc->GetTcpSock(), &QuesType, 1, 0);
      if(recvsize < 0)
      {
        LOG(ERROR, "Recv TagType failed") << std::endl;
        return NULL;
      }
      else if (recvsize == 0)
      {
        LOG(ERROR, "Client shutdown connect") << std::endl;
        return NULL;
      }

      uint32_t UserId = -1;
      int UserStatus = -1;
      //正常接收到一个请求标识
      switch(QuesType) 
      {
        case REGISTER:
          //使用用户管理模块注册:
          UserStatus = cs->DealRegister(lc->GetTcpSock(), &UserId);
          break;
        case LOGIN:
          //使用用户管理模块登录
          UserStatus = cs->DealLogin(lc->GetTcpSock());
          break;
        case LOGINOUT:
          //使用用户管理模块的退出登录
          cs->DeaLoginOut();
          break;
        default:
          UserStatus = REGFAILED;
          LOG(ERROR, "Recv Request Type not a effective value") << std::endl;
          break;

      }

      //响应 send；
      ReplyInfo ri;
      ri.Status = UserStatus;
      ri.UserId_ = UserId;
      ssize_t sendsize = send(lc->GetTcpSock(), &ri, sizeof(ri), 0);
      if(sendsize < 0)
      {
        //如果发送数据失败，是否需要考虑应用层重新发送一个
        LOG(ERROR, "SendMsg Failed") << std::endl;
      }
      LOG(INFO, "SendMsg success") << std::endl;

      //将TCP连接释放掉
      close(lc->GetTcpSock());
      delete lc;
      return NULL;
    }


    int DealRegister(int sock, uint32_t* UserId)
    {
        //接受注册请求
        RegInfo ri;
        ssize_t recvsize = recv(sock, &ri, sizeof(ri), 0);
        if(recvsize < 0)
        {
          LOG(ERROR, "Recv TagType failed") << std::endl;
          return OFFLINE;
        }
        else if(recvsize == 0)
        {
          LOG(ERROR, "Client shutdown connect") << std::endl;
            //特殊处理对端关闭的情况
        }
        //调用用户管理模块进行注册请求的处理
        int ret =  UserManager_->Register(ri.Nickname_, ri.School_, ri.Passwd_, UserId);
        //返回注册成功之后给用户的UserId
        if(ret == -1)
        {
          LOG(ERROR, "Register failed") << std::endl;
          return REGFAILED;
        }
        //返回当前状态
        LOG(INFO, "Register success") << std::endl;
        return REGISTERED;
    }

    int DealLogin(int sock)
    {
        struct LoginInfo li;
        ssize_t recvsize = recv(sock, &li, sizeof(li), 0);
        if(recvsize < 0)
        {
          LOG(ERROR, "Recv TagType failed") << std::endl;
          return OFFLINE;
        }
        else if(recvsize == 0)
        {
          LOG(ERROR, "Client shutdown connect") << std::endl;
          //需要处理
        }

        LOG(DEBUG, "UserID:Passwd") << li.UserId_ << ":" << li.Passwd_ << std::endl;
        int ret = UserManager_->Login(li.UserId_, li.Passwd_);
        if(ret == -1)
        {
          LOG(ERROR, "User login failed") << std::endl;
          return LOGINFAILED;
        }
        LOG(INFO, "Login success") << std::endl;
        return LOGINED;
    }

    int DeaLoginOut()
    {
      return 0;
    }

  private:
    void ReccMsg()
    {
      char buf[10240] = {0};
      struct sockaddr_in cliaddr;
      socklen_t cliaddrlen = sizeof (struct sockaddr_in);
      int recvsize = recvfrom(UdpSock_, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&cliaddr, &cliaddrlen);
      if (recvsize < 0)
      {
          LOG(ERROR, "recvfrom msg failed") << std::endl;
      }
      else
      {
         //正常逻辑
        std::string msg;
        msg.assign(buf, recvsize);
        LOG(INFO, msg) << std::endl;
        //需要将发送的数据从JSON格式转化为我们可以识别的数据
        Message jsonmsg;
        jsonmsg.Deserialize(msg);
        //需要增加用户管理，只有注册登录的人才可以向服务器发送消息
        //1.校验当前的消息是否属注册用户或者老用户发送到
        //   1.1不是，则认为是非法信息
        //   1.2是 区分是否第一次发送消息
        //      是：保存地址信息，并且更新状态为在线，将数据放到数据池
        //      是老用户：直接将数据放到数据池
        //2.需要校验，势必是和用户管理模块打交道
        bool ret = UserManager_->IsLogin(jsonmsg.GetUserId(), cliaddr, cliaddrlen);
        if(ret != true)
        {
          LOG(ERROR, "Discarded the msg") << std::endl;
        } 
        LOG(INFO, "Push msg to MsgPool") << std::endl;
        MsgPool_->PushMsgToPool(msg);
       }
    }

    void BroadcastMsg()
    {
      //1.获取要给哪个用户发送
      //2.获取发送内容
      std::string msg;
      MsgPool_->PopMsgFromPool(&msg);
      //用户管理系统提供在线用户的列表
      std::vector<UserInfo> OnlineUserVec;
      UserManager_->GetOnlineUserInfo(&OnlineUserVec);
      std::vector<UserInfo>::iterator iter = OnlineUserVec.begin();
      for(; iter != OnlineUserVec.end(); iter++)
      {
      //SendMsg()
          SendMsg(msg, iter->GetCliAddrInfo(), iter->GetCliAddrLen());
      }
    }

    //给一个客户端发送单个消息
    void SendMsg(const std::string& msg, struct sockaddr_in& cliaddr, socklen_t& len)
    {
     ssize_t sendsize =  sendto(UdpSock_, msg.c_str(), msg.size(), 0, (struct sockaddr*)&cliaddr, len);
     if(sendsize < 0)
     {
       LOG(ERROR, "sendto msg failed") << std::endl;
       //没有发送成功，我们是否需要缓存没有发送成功的信息还有客户端的地址
     }
     else
     {
       //成功
       LOG(INFO, "Sendto Msg success ") << "[" << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << "]" << msg << std::endl;
     }
    }
  private:
    int UdpSock_;
    int UdpPort_;
    MsgPool* MsgPool_;

    //tcp处理注册、登陆请求
    int TcpSock_;
    int TcpPort_;
    //用户管理模块
    UserManager* UserManager_;
};
