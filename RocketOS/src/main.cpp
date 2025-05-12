#include <Arduino.h>
#include "RocketOS.h"
#include <array>
// put function declarations here:
using namespace RocketOS;

class Cl{
  private:
    
  public:
  const Shell::CommandList* getList()const{
    return &m_commands;
  }



  /*Command Callbacks
  */
  
  void function1(const Shell::Token*){Serial.println("f1");}
  void function2(const Shell::Token*){Serial.println("f2");}
  void function3(const Shell::Token*){Serial.println("f3");}
  void function4(const Shell::Token*){Serial.println("f4");}

  void C1_function1(const Shell::Token*){Serial.println("c1f1");}
  void C1_function2(const Shell::Token*){Serial.println("c1f2");}
  void C1_function3(const Shell::Token*){Serial.println("c1f3");}

  void C2_function1(const Shell::Token*){Serial.println("c2f1");}
  void C2_function2(const Shell::Token*){Serial.println("c2f2");}
  void C2_def(const Shell::Token*){Serial.println("c2d");}

  void C3_function1(const Shell::Token*){Serial.println("c3f1");}
  void C3_function2(const Shell::Token*){Serial.println("c3f2");}
  void C3_function3(const Shell::Token*){Serial.println("c3f3");}
  void C3_def(const Shell::Token*){Serial.println("c3d");}


  void C3C1_function1(const Shell::Token*){Serial.println("c3c1f1");}
  void C3C1_function2(const Shell::Token*){Serial.println("c3c1f2");}

  private:
  /*CommandList Construction
  */
 //root command list commands------------------------
  const std::array<Shell::Command, 4> rootCommands = std::array{  
    Shell::Command{"func1", "u", [this](Shell::arg_t args){this->function1(args);}},
    Shell::Command{"func2", "f", [this](const Shell::Token* args){this->function2(args);}},
    Shell::Command{"func3", "u", [this](const Shell::Token* args){this->function3(args);}},
    Shell::Command{"func4", "u", [this](const Shell::Token* args){this->function4(args);}}
  };
    //child 1 command list commands-------------------
    const std::array<Shell::Command, 3> child1Commands = std::array{
      Shell::Command{"func1", "u", [this](const Shell::Token* args){this->C1_function1(args);}},
      Shell::Command{"func2", "fu", [this](const Shell::Token* args){this->C1_function2(args);}},
      Shell::Command{"func3", "u", [this](const Shell::Token* args){this->C1_function3(args);}}
    };
    //------------------------------------------------

    //child 2 command list commands-------------------
    const std::array<Shell::Command, 3> child2Commands = std::array{
      Shell::Command{"func1", "w", [this](const Shell::Token* args){this->C2_function1(args);}},
      Shell::Command{"func2", "u", [this](const Shell::Token* args){this->C2_function2(args);}},
      Shell::Command{"", "w", [this](const Shell::Token* args){this->C2_def(args);}}
    };
    //------------------------------------------------


    //child 3 command list commands-------------------
    const std::array<Shell::Command, 4> child3Commands = std::array{
      Shell::Command{"func1", "is", [this](const Shell::Token* args){this->C3_function1(args);}},
      Shell::Command{"func2", "s", [this](const Shell::Token* args){this->C3_function2(args);}},
      Shell::Command{"func3", "u", [this](const Shell::Token* args){this->C3_function3(args);}},
      Shell::Command{"", "", [this](const Shell::Token* args){this->C3_def(args);}}
    };
      //child 1 (of child3) command list commands---------
      const std::array<Shell::Command, 2> child3Child1Commands = std::array{
        Shell::Command{"func1", "", [this](const Shell::Token* args){this->C3C1_function1(args);}},
        Shell::Command{"", "u", [this](const Shell::Token* args){this->C3C1_function2(args);}}
      };
      //--------------------------------------------------
    //child 3 list of children
    const std::array<Shell::CommandList, 1> child3ChildrenList = std::array{
      Shell::CommandList{"Child1", child3Child1Commands.data(), child3Child1Commands.size(), nullptr, 0},
    };
    //------------------------------------------------
  
  //root command list children
  const std::array<Shell::CommandList, 3> children = std::array{
    Shell::CommandList{"Child1", child1Commands.data(), child1Commands.size(), nullptr, 0},
    Shell::CommandList{"Child2", child2Commands.data(), child2Commands.size(), nullptr, 0},
    Shell::CommandList{"Child3", child3Commands.data(), child3Commands.size(), child3ChildrenList.data(), child3ChildrenList.size()}
  };

  const Shell::CommandList m_commands = {"Name", rootCommands.data(), rootCommands.size(), children.data(), children.size()};
  //------------------------------------------------

};

class InterpreterTest{
  Cl obj;
  Shell::Interpreter interpreter;
public:
  InterpreterTest() : interpreter(obj.getList()){}
  void init(){
    interpreter.init();
  }
  void update(){
    interpreter.handleInput();
  }

};

InterpreterTest g_test;



void setup() {
  g_test.init();
}

void loop() {
  g_test.update();
  delay(250);
}
