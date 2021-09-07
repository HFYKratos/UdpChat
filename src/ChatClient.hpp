#pragma once 
#include <iostream>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <jsoncpp/json/json.h>

#include "LogSvr.hpp"
#include "ConnectInfo.hpp"

#define UDPPORT 17777
#define TCPPORT 17778

struct Myself
{
    std::string Nickname_;
    std::string School_;
    std::string Passwd_;
    uint32_t UserId_;
};



class ChatClient
{
    public:
        ChatClient(std::string SvrIp = "127.0.0.1")
        {
            UdpSock_ = -1;
            UdpPort_ = UDPPORT;
            TcpSock_ = -1;
            TcpPort_ = TCPPORT;

            SvrIP_ = SvrIp;
        }

        ~ChatClient()
        {
            if(UdpSock_ > 0)
            {
                close(UdpSock_);
            }
        }

        void Init()
        {
          UdpSock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
          if(UdpPort_ < 0)
          {
              LOG(ERROR, "Client create udp socket failed") << std::endl;
              exit(1);
          }
        }

    bool Connect2Server()
    {
        //创建TCP连接
        TcpSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(TcpSock_ < 0)
        {
           LOG(ERROR, "Client create tcp socket failed") << std::endl;
           exit(2);
        }
        //服务端的地址信息填充
        struct sockaddr_in peeraddr;
        peeraddr.sin_family = AF_INET;
        peeraddr.sin_port = htons(TcpPort_);
        peeraddr.sin_addr.s_addr = inet_addr(SvrIP_.c_str());

        int ret = connect(TcpSock_, (struct sockaddr*)&peeraddr, sizeof(peeraddr));
        if(ret < 0)
        {
          LOG(ERROR, "Connect Server failed") << SvrIP_ << ":" << TcpPort_ << std::endl;
          return false;
        }
        return true;
    }

    bool Login()
    {
      if(!Connect2Server())
      {
        return false;
      }
      //1.发送登陆标识
      char type = LOGIN;
      ssize_t send_size = send(TcpSock_, &type, sizeof(type), 0);
      if(send_size < 0)
      {
        LOG(ERROR, "Send Login Type failed") << std::endl;
        return false;
      }
      //2.发送登陆数据
      struct LoginInfo li;
      li.UserId_ = me_.UserId_;
      strcpy(li.Passwd_, me_.Passwd_.c_str());

      send_size = send(TcpSock_, &li, sizeof(li), 0);
      if(send_size < 0)
      {
        LOG(ERROR, "Send Login Informations failed") << std::endl;
        return false;
      }
      //3.解析登陆状态
      struct ReplyInfo resp;
      ssize_t recv_size = recv(TcpSock_, &resp, sizeof(resp), 0);
      if(recv_size < 0)
      {
        LOG(ERROR, "Recvice respone failed") << std::endl;
        return false;
      }
      else if(recv_size == 0)
      {
        LOG(ERROR, "Peer shutdown connect") << std::endl;
        return false;
      }

      if(resp.Status != LOGINED)
      {
        LOG(ERROR, "Login failed, Status is ") << resp.Status << std::endl;
        return false;
      }
      LOG(INFO, "Login success") << std::endl;
      return true;
    }

    bool Register()
    {
      if(!Connect2Server())
      {
        return false;
      }
      //1.发送注册标识 
      char type = REGISTER;
      ssize_t send_size = send(TcpSock_, &type, 1, 0);
      if(send_size < 0)
      {
        LOG(ERROR,"Send Register Type failed") << std::endl;
        return false;
      }
      //2.发送注册内容
      struct RegInfo ri;
      std::cout << "Please Enter Your Nickname:";
      std::cin >> ri.Nickname_;
      std::cout << "Please Enter Your School:";
      std::cin >> ri.School_;
      while(1)
      {
         std::cout <<  "Please Enter Your Password:";
         std::string PasswdOne;
         std::cin >> PasswdOne;
         std::cout << "Please Enter Your Password again:";
         std::string PasswdTwo;
         std::cin >> PasswdTwo;

         if(PasswdOne == PasswdTwo)
         {
           strcpy(ri.Passwd_, PasswdOne.c_str());
           break;
         }
         else
         {
            printf("The Password did not match.");
         }
      }
      
      send_size = send(TcpSock_, &ri, sizeof(ri), 0);
      if(send_size < 0)
      {
        LOG(ERROR, "Send Register Informations failed") << std::endl;
        return false;
      }
      //3.解析应答状态和获取用户ID
      struct ReplyInfo resp;
      ssize_t recv_size = recv(TcpSock_, &resp, sizeof(resp), 0);
      if(recv_size < 0)
      {
        LOG(ERROR, "Recvice Register respone failed") << std::endl;
        return false;
      }
      else if(recv_size == 0)
      {
        LOG(ERROR, "Peer ShutDown connect") << std::endl;
        return false;
      }

      if(resp.Status != REGISTERED)
      {
        LOG(ERROR, "Register failed") << std::endl;
        return false;
      }
      LOG(INFO,"Register success, UserId is ") << resp.UserId_ << std::endl;
      //保存用户信息
      me_.Nickname_ = ri.Nickname_;
      me_.School_ = ri.School_;
      me_.Passwd_ = ri.Passwd_;
      me_.UserId_ = resp.UserId_;
      close(TcpSock_);
      return true;
    }

    //发送正常的业务数据
    bool SendMsg(const std::string& msg)
    {
      struct sockaddr_in peeraddr;
      peeraddr.sin_family = AF_INET;
      peeraddr.sin_port = htons(UdpPort_);
      peeraddr.sin_addr.s_addr = inet_addr(SvrIP_.c_str());
      ssize_t send_size =  sendto(UdpSock_, msg.c_str(), msg.size(), 0, (struct sockaddr*)&peeraddr, sizeof(peeraddr));
      if(send_size < 0)
      {
        LOG(ERROR, "Send Msg to Server Failed") << std::endl;
        return false;
      }
      return true;
    }

    bool RecvMsg(std::string* msg)
    {
      char buf[MESSAGE_MAX_SIZE];
      memset(buf, '\0', sizeof(buf));
      struct sockaddr_in svraddr;
      socklen_t svraddrlen = sizeof(svraddr);
      ssize_t recv_size = recvfrom(UdpSock_, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&svraddr, &svraddrlen);
      if(recv_size < 0)
     {
       LOG(ERROR, "Recvice message failed") << std::endl;
        return false;
      }
      (*msg).assign(buf, recv_size);
      return true;
    }

    Myself& GetMyself()
    {
      return me_;
    }

    void PushUser(std::string& user_info)
    {
      auto iter = OnlineUserList_.begin();
      for(; iter != OnlineUserList_.end(); iter++)
      {
        if(*iter == user_info)
        {
          return;
        }
      }

      OnlineUserList_.push_back(user_info);
      return;
    }

    std::vector<std::string>& GetOnlineUserList()
    {
      return OnlineUserList_;
    }

    private:
        //发送正常的业务数据
        int UdpSock_;
        int UdpPort_;

        //处理登陆注册请求
        int TcpSock_;
        int TcpPort_;

        //保存服务端的ip
        std::string SvrIP_;
        //客户端的信息
        Myself me_;

        //当前在线用户
        std::vector<std::string> OnlineUserList_;
};
