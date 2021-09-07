#pragma once 
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <vector>
#include <stdlib.h>

#include "ChatClient.hpp"
#include "Message.hpp"

class ChatWindow;

class Param
{
    public:
      Param(ChatWindow* winp, int ThreadNumber, ChatClient* ChatCli)
      {
          Winp_ = winp;
          ThreadNumber_ = ThreadNumber;
          ChatCli_ = ChatCli;
      }

    public:
      ChatWindow* Winp_;
      int ThreadNumber_;

      ChatClient* ChatCli_;
};

class ChatWindow
{
    public:
      ChatWindow()
      {
        header_ = NULL;
        output_ = NULL;
        user_list_ = NULL;
        input_ = NULL;
        threads_.clear();
        pthread_mutex_init(&lock_, NULL);
        initscr();
        curs_set(0);
      }

      ~ChatWindow()
      {
        if(header_)
        {
          delwin(header_);
        }
        if(output_)
        {
          delwin(output_);
        }
        if(user_list_)
        {
          delwin(user_list_);
        }
        if(input_)
        {
          delwin(input_);
        }
        endwin();
        pthread_mutex_destroy(&lock_);
      }

      void DrawHeader()
      {
        int lines = LINES / 5;
        int cols = COLS;
        int start_y = 0;
        int start_x = 0;
        header_ = newwin(lines, cols, start_y, start_x);
        box(header_, 0, 0);
        pthread_mutex_lock(&lock_);
        wrefresh(header_);
        pthread_mutex_unlock(&lock_);
      }
      
      void DrawOutput()
      {
        int lines = (LINES * 3) / 5;
        int cols = (COLS * 3) / 4;
        int start_y = LINES / 5;
        int start_x = 0;
        output_ = newwin(lines, cols, start_y, start_x);
        box(output_, 0, 0);
        pthread_mutex_lock(&lock_);
        wrefresh(output_);
        pthread_mutex_unlock(&lock_);
      }

      void DrawUserList()
      {
        int lines = (LINES * 3) / 5;
        int cols = COLS / 4;
        int start_y = LINES / 5;
        int start_x = (COLS * 3) / 4;
        user_list_ = newwin(lines, cols, start_y, start_x);
        box(user_list_, 0, 0);
        pthread_mutex_lock(&lock_);
        wrefresh(user_list_);
        pthread_mutex_unlock(&lock_);
      }

      void DrawInput()
      {
        int lines = LINES / 5;
        int cols = COLS;
        int start_y = (LINES * 4) / 5;
        int start_x = 0;
        input_ = newwin(lines, cols, start_y, start_x);
        box(input_, 0, 0);
        pthread_mutex_lock(&lock_);
        wrefresh(input_);
        pthread_mutex_unlock(&lock_);
      }

      void PutStringToWin(WINDOW* win, int y, int x, std::string& msg)
      {
          mvwaddstr(win, y, x, msg.c_str());
          pthread_mutex_lock(&lock_);
          wrefresh(win);
          pthread_mutex_unlock(&lock_);
      }

      void GetStringFromWin(WINDOW* win, std::string* data)
      {
        char buf[1024];
        memset(buf, '\0', sizeof(buf));
        wgetnstr(win, buf, sizeof(buf) - 1);
        (*data).assign(buf, strlen(buf));
      }

      static void RunHeader(ChatWindow* cw)
      {
        //1.绘制窗口 
        //2.展示欢迎语
        std::string msg = "Welcome to our Chat System";
        int x, y;
        size_t pos = 1;
        //标识是否需要改变输出的方向  本身是想向右输出，到最右之后，想向左输出
        //0：可以继续向右输出
        //1：需要向左输出
        int flag = 0;
        while(1)
        {
          cw->DrawHeader();
          getmaxyx(cw->header_, y, x);
          cw->PutStringToWin(cw->header_, y / 2, pos, msg);
          if(pos < 2)
          {
             flag = 0;
          }
          else if(pos > x - msg.size() - 2)//右边界
          {
              flag = 1;
          }

          if(flag == 0)
          {
            pos++;
          }
          else 
          {
            pos--;
          }
          sleep(1);
        }
      }

      static void RunOutput(ChatWindow* cw, ChatClient* cc)
      {
        std::string recv_msg;
        Message msg;
        cw->DrawOutput();
        int line = 1;
        int y, x;
        while(1)
        {
          getmaxyx(cw->output_, y, x);
          cc->RecvMsg(&recv_msg);
          //反序列化
          msg.Deserialize(recv_msg);
          //展示数据 昵称-学校# 消息
          std::string show_msg;
          show_msg += msg.GetNickname();
          show_msg += "-";
          show_msg += msg.GetSchool();
          show_msg += "# ";
          show_msg += msg.GetMsg();
          if(line > y - 2)
          {
            line = 1;
            cw->DrawOutput();
          }
          cw->PutStringToWin(cw->output_, line, 1, show_msg);
          line++;

          std::string user_info;
          user_info += msg.GetNickname();
          user_info += "-";
          user_info += msg.GetSchool();

          cc->PushUser(user_info);
        }
      }

      static void RunUserList(ChatWindow* cw, ChatClient* cc)
      {
        int x, y;

        while(1)
        {
          cw->DrawUserList();
          getmaxyx(cw->user_list_, y, x);
          auto UserList = cc->GetOnlineUserList();
          for(auto& iter : UserList)
          {
            int line =  1;
            cw->PutStringToWin(cw->user_list_, line++, 1, iter);
          }
          sleep(1);
        }
      }

      static void RunInput(ChatWindow* cw, ChatClient* cc)
      {
        //昵称 学校 msg 用户ID
        Message msg; 
        //设置昵称等
        msg.SetNickname(cc->GetMyself().Nickname_);
        msg.SetSchool(cc->GetMyself().School_);
        msg.SetUserId(cc->GetMyself().UserId_);
        //用户输入的原始信息
        std::string User_enter_msg;
        std::string tips = "Please Enter# ";
        std::string send_msg;
        while(1)
        {
          cw->DrawInput();
          cw->PutStringToWin(cw->input_, 2, 2, tips);
          //从窗口当中获取数据，放到send_msg当中
          cw->GetStringFromWin(cw->input_, &User_enter_msg);
          msg.SetMsg(User_enter_msg);

          msg.Serialize(&send_msg);
          cc->SendMsg(send_msg);
        }
      }

      static void* DrawWindow(void* arg)
      {
        Param* pm =  (Param*)arg;
        ChatWindow* cw = pm->Winp_;
        ChatClient* cc = pm->ChatCli_;
        int thread_num = pm->ThreadNumber_;
        switch(thread_num)
        {
          case 0:
            RunHeader(cw);
            break;
          case 1:
            RunOutput(cw, cc);
            break;
          case 2:
            RunUserList(cw, cc);
            break;
          case 3:
            RunInput(cw, cc);
            break;
          default:
            break;
        }
        delete pm;
        return NULL;
      }

      void Start(ChatClient* chatcli)
      {
        int i = 0;
        pthread_t tid;
        for(; i < 4; i++)
        {
          Param* pm = new Param(this, i, chatcli);
          int ret = pthread_create(&tid, NULL, DrawWindow, (void*)pm);
          if(ret < 0)
          {
              printf("Create thread failed");
              exit(1);
          }
          threads_.push_back(tid);
        }
        for(i = 0; i < 4; i++)
        {
          pthread_join(threads_[i], NULL);
        }
        
      }

    private:
      WINDOW* header_;
      WINDOW* output_;
      WINDOW* user_list_;
      WINDOW* input_;

      std::vector<pthread_t> threads_;
      pthread_mutex_t lock_;
};
