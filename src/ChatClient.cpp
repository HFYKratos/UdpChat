#include "ChatClient.hpp"
#include "ChatWindow.hpp"

void Menu()
{
  std::cout << "---------------------------" << std::endl;
  std::cout << "| 1.register      2.login |" << std::endl;
  std::cout << "|                         |" << std::endl;
  std::cout << "| 3.loginout      4.exit  |" << std::endl;
  std::cout << "---------------------------" << std::endl;
}

int main(int argc, char* argv[])
{
  // ./ChatCli [ip]
  if(argc != 2)
  {
    printf("./ChatCli [ip]\n");
    exit(1);
  }

  ChatClient* cc = new ChatClient(argv[1]);
  //1.初始化服务
  cc->Init();
  while(1)
  {
    Menu();
    int Select = -1;
    std::cout << "Please Select service: ";
    fflush(stdout);
    std::cin >> Select;
    //2.注册
    if(Select == 1)
    {
        if(!cc->Register())
        {
          std::cout << "Register failed! Please Try again!" << std::endl;
        }
        else 
        {
          std::cout << "Register success! Please Login!" << std::endl;
        }
    }
    //3.登陆
    else if(Select == 2)
    {
        if(!cc->Login())
        {
          std::cout << "Login failed! Please check your UseId or Password!" << std::endl;
        }
        else
        {
          std::cout << "Login success! Please Chat Online!" << std::endl;
          ChatWindow* cw = new ChatWindow();
          cw->Start(cc);
        }
    }

    else if (Select == 3)
    {
      //退出登录
    }
    else if (Select == 4)
    {
      //退出
      break;
    }
    //4.发送&接收数据
  }
  delete cc;
  return 0;
}
