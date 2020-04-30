#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "scanner.h"
#include "parse_error.h"

using namespace std;

typedef vector<Token> TokenLine;
typedef vector<TokenLine> AssemblyCode;
typedef unordered_map<string, int> Dictionary;

// Declaration of the constant values
const short HEXADECIMAL_MIN = 0;
const int DECIMAL_MIN = -2147483648;
const unsigned int DECIMAL_MAX = 4294967295;
const unsigned int HEXADECIMAL_MAX = 4294967295;
const short HEXADECIMAL_IMMEDIATE_MIN = 0;
const unsigned short HEXADECIMAL_IMMEDIATE_MAX = 65535;
const short DECIMAL_IMMEDIATE_MIN = -32768;
const short DECIMAL_IMMEDIATE_MAX = 32767;
const short LABEL_IMMEDIATE_MIN = -32768;
const short LABEL_IMMEDIATE_MAX = 32767;

// prints [value] in binary digits
void printNumber(long value) {
  char currentValueToPrint;
  // print the value byte by byte
  for (int i = 0; i < 4; i++) {
    int bitsToShift = 24 - 8 * i;
    currentValueToPrint = value >> bitsToShift;
    cout << currentValueToPrint;
  }
}

// checks if the hexadecimal number is in range
// range: [0, 2^32 - 1]
// range: [0, 4294967295]
bool hexadecimalNumberIsInRange(long num) {
  return (HEXADECIMAL_MIN <= num && num <= HEXADECIMAL_MAX);
}

// checks if the decimal number is in range
// range: [-2^31, 2^32 - 1]
// range: [-2147483648, 4294967295]
bool decimalNumberIsInRange(long num) {
  return (DECIMAL_MIN <= num && num <= DECIMAL_MAX);
}

// checks if the label is not previously defined
bool labelIsNotPreviouslyDefined(string labelName, Dictionary &table) {
  return (table.find(labelName) == table.end());
}

// output the symbol table
void outputSymbolTable(Dictionary &t) {
  for (auto label: t) {
    cerr << label.first << " " << label.second << endl;
  }
}

// checks if the register number is in range
bool registerNumberInRange(const int registerNumber) {
  return (registerNumber >= 0) && (registerNumber <= 31);
}

// checks if the hexadecimal immediate is out of bound
bool hexadecimalImmediateOutOfBound(long long number) {
  return (number > HEXADECIMAL_IMMEDIATE_MAX) || (number < HEXADECIMAL_IMMEDIATE_MIN);
}

// checks if the decimal immediate is out of bound
bool decimalImmediateOutOfBound(long long number) {
  return (number > DECIMAL_IMMEDIATE_MAX) || (number < DECIMAL_IMMEDIATE_MIN);
}

// checks if the label immediate is out of bound
bool labelImmediateOutOfBound(long long label) {
  return (label > LABEL_IMMEDIATE_MAX) || (label < LABEL_IMMEDIATE_MIN);
}

int main() {
  string line;
  int PC = 0;
  Dictionary symbolTable;
  AssemblyCode code;
  Dictionary opcode;

  // Define the opcode for the mips functions
  opcode["add"] = 0x20;
  opcode["sub"] = 0x22;
  opcode["slt"] = 0x2a;
  opcode["sltu"] = 0x2b;
  opcode["jr"] = 0x8;
  opcode["jalr"] = 0x9;
  opcode["beq"] = 0x4;
  opcode["bne"] = 0x5;
  opcode["lis"] = 0x14;
  opcode["mfhi"] = 0x10;
  opcode["mflo"] = 0x12;
  opcode["mult"] = 0x18;
  opcode["multu"] = 0x19;
  opcode["div"] = 0x1a;
  opcode["divu"] = 0x1b;
  opcode["lw"] = 0x23;
  opcode["sw"] = 0x2b;

  try {
    // First Pass
    while (getline(cin, line)) { // First pass to deal with labels
      TokenLine tokenLine = scan(line); // Store the assembly code into Tokens
      TokenLine secondPass;
      // If the line is empty, go to next line.
      if (tokenLine.empty()) continue;
    
      // Loop through the line of code
      for (auto &token: tokenLine) {
        if (token.getKind() == Token::LABEL) { // The current token is a label
          if (secondPass.empty()) {
            int labelLength = token.getLexeme().size();
            string labelName = token.getLexeme().substr(0, labelLength - 1);
            if (labelIsNotPreviouslyDefined(labelName, symbolTable)) {
              symbolTable[labelName] = PC;
            } else {
              throw ParseError("(1st Pass): Label \"" + labelName + "\" is previously defined.");
            }
          } else {
            throw ParseError("(1st Pass): Label is not the first element of a line.");
          }
        } else { // store any tokens other than "label"
          secondPass.emplace_back(token);
        }
      }
      if (!secondPass.empty()) { // whenever the line is not empty
        code.emplace_back(secondPass);
        PC += 4;
      }
    }

    int PCSecondPass = 0;
    
    // Second Pass
    for (auto &codeLine: code) {
      auto &token = codeLine[0];
      if (token.getKind() == Token::WORD) { // Parse a word
        // Reject any code in the form [.word 1232 124124]
        if (codeLine.size() != 2) {
          throw ParseError("(1st Pass): Incorrect number of parameters after \".word\" keyword.");
        }
        token = codeLine[1];
        // Parse [.word HEXAINT]
        if (token.getKind() == Token::HEXINT) {
          // convert the hexadecimal number into an integer
          const unsigned int hexaValue = token.toLong();
          if (hexadecimalNumberIsInRange(hexaValue)) {
            printNumber(hexaValue);
          } else {
            throw ParseError("(1st Pass): Hexadecimal number NOT IN RANGE");
          }
        // Parse [.word INT]
        } else if (token.getKind() == Token::INT) {
          // convert the decimal number into an integer
          const long decimalValue = token.toLong();
          if (decimalNumberIsInRange(decimalValue)) {
            printNumber(decimalValue);
          } else {
            throw ParseError("(2nd Pass) Decimal number NOT IN RANGE");
          }
        } else if (token.getKind() == Token::ID) {
          try {
            int value = symbolTable.at(token.getLexeme());
            printNumber(value);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass) Attempt to retrive an undefined label.");
          }
        } else {
            throw ParseError("(2nd Pass) Unexpected token type following .word");
        }
      } else if (token.getKind() == Token::ID) { // Parse a token
        if (token.getLexeme() == "jr" || token.getLexeme() == "jalr") { // jr/jalr
          try {
            if (codeLine.size() != 2) {
              throw ParseError("(2nd Pass): Incorrect number of paramaters after JR/JALR Command");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            token = codeLine[1]; 
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): Element followed JR/JALR is not a rigister");
            }
            short registerNum = token.toLong();
            if (!registerNumberInRange(registerNum)) {
              throw ParseError("(2nd Pass):  Register Number followed JR/JALR command is out of bound");
            }        
            // output the binary code of jr/jalr command
            int binaryInstruction = (0 << 26) | (registerNum << 21) | (0 << 16) | (opcodeForCommand & 0xffff);
            printNumber(binaryInstruction);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass): Attempt to access undefined element");
          }
        } else if (token.getLexeme() == "add" || token.getLexeme() == "sub" || token.getLexeme() == "slt" || token.getLexeme() == "sltu") { // parse "add", "sub", "slt", "sltu"
          try {
            if (codeLine.size() != 6) {
              throw ParseError("(2nd Pass): Incorrect number of paramters following add/sub/slt/sltu");
            }
            if (codeLine[2].getKind() != Token::COMMA || codeLine[4].getKind() != Token::COMMA) {
              throw ParseError("(2nd Pass): add/sub/slt/slut[2] and [4] should be comma(s).");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            token = codeLine[1];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): add/sub/slt/sltu[1] should be a register.");
            }
            long long valueOfRegisterD = token.toLong();
            if (!registerNumberInRange(valueOfRegisterD)) {
              throw ParseError("(2nd Pass): Register D of add/sub/slt/sltu is out of range.");
            }
            token = codeLine[3];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): add/sub/slt/sltu[3] should be a register.");
            }
            long long valueOfRegisterS = token.toLong();
            if (!registerNumberInRange(valueOfRegisterS)) {
              throw ParseError("(2nd Pass): Register S of add/sub/slt/sltu is out of range.");
            }
            token = codeLine[5];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): add/sub/slt/sltu[5] should be a register.");
            }
            long long valueOfRegisterT = token.toLong();
            if (!registerNumberInRange(valueOfRegisterT)) {
              throw ParseError("(2nd Pass): Register T of add/sub/slt/sltu is out of range.");
            }
            // print the binary code for add/sub/slt/sltu
            int instructionCode = (0 << 26) | (valueOfRegisterS << 21) | (valueOfRegisterT << 16) | (valueOfRegisterD << 11) | (opcodeForCommand & 0xffff);
            printNumber(instructionCode);
          } catch(out_of_range) {
            throw ParseError("(2nd Pass): Attempt to access undefined label in an add/sub/slt/sltu command.");
          }
        } else if (token.getLexeme() == "beq" || token.getLexeme() == "bne") { // beq/bne
          try {
            if (codeLine.size() != 6) {
              throw ParseError("(2nd Pass): Incorrect number of paramters following beq/bne.");
            }
            if (codeLine[2].getKind() != Token::COMMA || codeLine[4].getKind() != Token::COMMA) {
              throw ParseError("(2nd Pass): Expecting a comma after an beq/bne command.");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            
            token = codeLine[1];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): beq/bne[1] should be a register.");
            }
            long long valueOfRegisterS = token.toLong();
            if (!registerNumberInRange(valueOfRegisterS)) {
              throw ParseError("(2nd Pass): Register D of beq/bne is out of range.");
            }
            token = codeLine[3];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): beq/bne[3] should be a register.");
            }
            long long valueOfRegisterT = token.toLong();
            if (!registerNumberInRange(valueOfRegisterT)) {
              throw ParseError("(2nd Pass): Register S of beq/bne is out of range.");
            }
            token = codeLine[5];
            long long valueI = 0;
            if (token.getKind() == Token::INT) {
              valueI = token.toLong();
              if (decimalImmediateOutOfBound(valueI)) {
                throw ParseError("(2nd Pass): Decimal immediate after beq/bne is out of range.");
              }
            } else if (token.getKind() == Token::HEXINT) {
              valueI = token.toLong();
              if (hexadecimalImmediateOutOfBound(valueI)) {
                throw ParseError("(2nd Pass): Hexadecimal immediate after beq/bne is out of range.");
              }
            } else if (token.getKind() == Token::ID) {
              valueI = (symbolTable.at(token.getLexeme()) - PCSecondPass - 4) / 4;
              if (labelImmediateOutOfBound(valueI)) {
                throw ParseError("(2nd Pass) Label immediate after beq/bne is out of range.");
              }
            } else {
              throw ParseError("(2nd Pass): Unexpected token type at beq/bne[5]");
            }
            int instructionCode = (opcodeForCommand << 26) | (valueOfRegisterS << 21) | (valueOfRegisterT << 16) | (valueI & 0xffff);
            printNumber(instructionCode);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass): Attempt to jump to undefined label after beq/bne");
          }
        } else if (token.getLexeme() == "lis" || token.getLexeme() == "mfhi" || token.getLexeme() == "mflo") { // parse mfhi, mflo, and lis
          try {
            if (codeLine.size() != 2) {
              throw ParseError("(2nd Pass): Incorrect number of token after lis/mfhi/mflo command.");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            token = codeLine[1];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): Token following lis/mfhi/mflo is not a register.");
            }
            long long registerNum = token.toLong();
            if (!registerNumberInRange(registerNum)) {
              throw ParseError("(2nd Pass): Register number following lis/mfhi/mflo is out of range.");
            }
            int instructionCode = (0 << 16) | (registerNum << 11) | (opcodeForCommand & 0xffff);
            printNumber(instructionCode);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass): Attempt to use undefined label in lis/mfhi/mflo.");
          }
        } else if (token.getLexeme() == "mult" || token.getLexeme() == "multu" || token.getLexeme() == "div" || token.getLexeme() == "divu") { // parse mult/multu/div/divu
          try {
            if (codeLine.size() != 4) {
              throw ParseError("(2nd Pass): Incorrect number of tokens following mult/multu/div/divu.");
            }
            if (codeLine[2].getKind() != Token::COMMA) {
              throw ParseError("(2nd Pass): mult/multu/div/divu[2] should be a comma.");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            token = codeLine[1];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): mult/multu/div/divu[1] should be a register.");
            }
            long long valueOfRegisterS = token.toLong();
            if (!registerNumberInRange(valueOfRegisterS)) {
              throw ParseError("(2nd Pass): Register S of mult/multu/div/divu is out of range.");
            }
            token = codeLine[3];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): mult/multu/div/divu[3] should be a register.");
            }
            long long valueOfRegisterT = token.toLong();
            if (!registerNumberInRange(valueOfRegisterT)) {
              throw ParseError("(2nd Pass): Register T of mult/multu/div/divu is out of range.");
            }
            int instructionCode = (0 << 26) | (valueOfRegisterS << 21) | (valueOfRegisterT << 16) | (0 << 6) | (opcodeForCommand & 0xffff);
            printNumber(instructionCode);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass): Attempt to use undefined label in mult/multu/div/divu.");
          }
        } else if (token.getLexeme()== "lw" || token.getLexeme() == "sw") { // parse lw/sw
          try {
            if (codeLine.size() != 7) {
              throw ParseError("(2nd Pass): Incorrect number of tokens after lw/sw command.");
            }
            // form: lw $1 0($2)
            if ((codeLine[2].getKind() != Token::COMMA) || (codeLine[4].getKind() != Token::LPAREN) || (codeLine[6].getKind() != Token::RPAREN)) {
              throw ParseError("(2nd Pass): The form of lw/sw command is incorrect.");
            }
            int opcodeForCommand = opcode.at(token.getLexeme());
            token = codeLine[1];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): lw/sw[1] should be a register.");
            }
            long long valueOfRegisterT = token.toLong();
            if (!registerNumberInRange(valueOfRegisterT)) {
              throw ParseError("(2nd Pass): Register T of lw/sw is out of range.");
            }
            token = codeLine[3];
            long long valueOfImmediate = 0;
            if (token.getKind() == Token::INT) {
              valueOfImmediate = token.toLong();
              if (decimalImmediateOutOfBound(valueOfImmediate)) {
                throw ParseError("(2nd Pass): Decimal Immediate of lw/sw is out of range.");
              }
            } else if (token.getKind() == Token::HEXINT) {
              valueOfImmediate = token.toLong();
              if (hexadecimalImmediateOutOfBound(valueOfImmediate)) {
                throw ParseError("(2nd Pass): Hexadecimal immediate of lw/sw is out of range.");
              }  
            } else {
              throw ParseError("(2nd Pass): Expecting a decimal/hexadecimal immediate at lw/sw[3].");
            }
            token = codeLine[5];
            if (token.getKind() != Token::REG) {
              throw ParseError("(2nd Pass): lw/sw[5] should be a register.");
            }
            long long valueOfRegisterS = token.toLong();
            if (!registerNumberInRange(valueOfRegisterS)) {
              throw ParseError("(2nd Pass): Register S of lw/sw is out of range.");
            }
            int instructionCode = (opcodeForCommand << 26) | (valueOfRegisterS << 21) | (valueOfRegisterT << 16) | (valueOfImmediate & 0xffff);
            printNumber(instructionCode);
          } catch (out_of_range) {
            throw ParseError("(2nd Pass): Attempt to use undefined label in lw/sw");
          }
        } else {
          throw ParseError("(2nd Pass) Unexpected token type of ID.");
        }
      } else {
        throw ParseError("(2nd Pass): Unexpected token type other than [word] or [ID].");
      }
      PCSecondPass += 4;
    }
  } catch (ScanningFailure &f) {
    cerr << "Scanning Failed" << f.what() << endl;
    return 1;
  } catch (ParseError e) {
    cerr << "ERROR: " << e.getMessage() << endl;
    return 1;
  }

  outputSymbolTable(symbolTable);
  return 0;
}
