
#include <bitset>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// TYPE DECLARATIONS

using Operation = struct{
    std::string opCode;
    uint8_t code;
    bool extraLong;
    int reg1;
    int reg2;
    int value;
    std::string label;  // Assume that you can't jump to a jump ( only 1 label )
};

using Record = struct {
    uint16_t addr;
    std::string inputLine;
    Operation operation;
};

// Fields filled in with REQUIRED/NONE:
const int REQUIRED{-1};
const int NONE{-2};
const std::string HAS_LABEL{"LABEL"};
const std::string NO_LABEL{""};

using Program = std::vector<Record>;
using OperationList = std::vector<Operation>;

// We don't 'need' to assign values to these enums but we will since we care about their value
enum Opcode {
    OPCODE_LD   = 0x00,
    OPCODE_ST   = 0x01,
    OPCODE_COPY = 0x02,
    OPCODE_SWAP = 0x03,
    OPCODE_JMP  = 0x04,
    OPCODE_ADD  = 0x05,
    OPCODE_SUB  = 0x06,
    OPCODE_ADDC = 0x07,
    OPCODE_SUBC = 0x08,
    OPCODE_NOT  = 0x09,
    OPCODE_AND  = 0x0A,
    OPCODE_OR   = 0x0B,
    OPCODE_SRA  = 0x0C,
    OPCODE_RRC  = 0x0D,
};

// Array of strings containing all the Op Codes in their respective order. IN and OUT will become VADD and VSUB later.
const OperationList OPCODE_LIST = {
    // Opcode        code    // ExtraLong     R1          R2            Value   Label
    { "LD",     OPCODE_LD,   true,           REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    { "ST",     OPCODE_ST,   true,           REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    { "CPY",    OPCODE_COPY, false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SWAP",   OPCODE_SWAP, false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "JMP",    OPCODE_JMP,  true,           REQUIRED,   NONE,       NONE,      HAS_LABEL },
    { "ADD",    OPCODE_ADD,  false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SUB",    OPCODE_SUB,  false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "ADDC",   OPCODE_ADDC, false,          REQUIRED,   NONE,       REQUIRED,  NO_LABEL  },
    { "SUBC",   OPCODE_SUBC, false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "NOT",    OPCODE_NOT,  false,          REQUIRED,   NONE,       NONE,      NO_LABEL  },
    { "AND",    OPCODE_AND,  false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "OR",     OPCODE_OR,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SRA",    OPCODE_SRA,  false,          REQUIRED,   NONE,       REQUIRED,  NO_LABEL  },
    { "RRC",    OPCODE_RRC,  false,          REQUIRED,   NONE,       REQUIRED,  NO_LABEL  }
    // { "IN",       0x0E,   false,          REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    // { "OUT",      0x0F,   false,          REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
};

// FUNCTION DECLARATIONS

// Parse the file
// Codetags are '.code' & '.endcode'  To run without them change to false
using FileVector = std::vector<std::string>;
FileVector ParseFile (std::string &fileName, bool useCodeTags = true);

// Trim the input line 
std::string TrimInput(const std::string &input);

// Parses line input.
// Token is filled in with the first token.
// Returned string is the remaining string after the token (minus whitespace) or empty string
std::string Token(const std::string &input, std::string &token);

// Parse a line for an operation
bool ParseOperation(const std::string &input, Operation &operation);

// From our program generate the assembly.
// Return true if we generate the file.
bool GenerateAssembly (const std::string &fileName, const Program &program);

// Search our OPCODE Table for token.  If we find it, opcodeRule points the the row in the table and we return true.
// Otherwise return false
bool LookupOperation (const std::string &token, Operation &opcodeRule);

// Parse the token for a register value  'R#'
// Return true if value filled in
bool ParseRegister(const std::string &token, int &value);

// Parse the token for a  value  '#'
// Return true if value filled in
bool ParseNumeric(const std::string &token, int &value);

// Parse a register offset
// return true if register and value filled in
bool ParseRegisterOffset(const std::string &token, int &reg, int &value);

// Scan the program to determine the offset for a label from a addr where jmp is located.
bool GetJumpOffset( const Program &program, const std::string &label, uint16_t fromAddr, uint16_t &offset );

// Convert a 16 bit value to hex string
std::string ToString( uint16_t addr );

//*************************************************************************************************
int main(int argc, char** argv) {
    bool success{true};
    std::string fileName{"dxp_dcs.txt"};
    std::string outputFile{"dxp_dcs.mif"};
    FileVector fileVector = ParseFile(fileName);

    Program program;
    int cnt{1};
    uint16_t addr{};
    for ( const auto line : fileVector) {
        std::cout << "LINE " << std::to_string(cnt) << ": '" << line  << "'" << std::endl;
       Operation operation{};
       if (ParseOperation(line, operation)) {
            Record record{};
            record.addr = addr;
            record.inputLine = line;
            record.operation = operation;
            program.push_back(record);
            addr+=record.operation.extraLong ? 2 : 1;
       } else {
           std::cout << "SYNTAX ERROR: '" << line << "'" << std::endl;
           success = false;
           break;
       }
       ++cnt;
    }

    if (success) {
        std::cout << "Processing Complete..." << std::endl;
        success = GenerateAssembly(outputFile, program);

        if (!success) {
            std::cout << "FAILED TO GENERATE ASSEMBLY" << std::endl;
        }
    }
    if (success) {
        std::cout << "OUTPUT FILE " << outputFile << std::endl;
    }
    return success ? 0 : -1;
}


//*************************************************************************************************
FileVector ParseFile (std::string &fileName, bool useCodeTags) {
    FileVector fileVector;
    std::ifstream myFile;

    const std::string codeStart{".code"};
    const std::string codeEnd{".endcode"};

    myFile.open(fileName);
    
    if (myFile.is_open()) {
        // This loop will run until it reaches the end of the file
        bool codeBlock{!useCodeTags};
        while (!myFile.eof()) {
            std::string currentLine;
            std::getline(myFile, currentLine); // This will read the line from the file and store it into a designated string
            if (codeBlock) {
                if (currentLine.find(codeEnd) != std::string::npos) {
                    std::cout << "Processing... [.endcode]" << std::endl;
                    codeBlock = false;
                } else {
                    fileVector.push_back(TrimInput(currentLine));
                }
            }
            else if (currentLine.find(codeStart) != std::string::npos) {
                    std::cout << "Processing... [.code]" << std::endl;
                    codeBlock = true;
                }
        }
       myFile.close(); // Closing the file after completion
    } else {
        std::cout << "File could not be opened: " << fileName << std::endl;
    }
    return fileVector;
}

//*************************************************************************************************
std::string TrimInput(const std::string &input) {
    // Remove preceding whitespace
    // Remove semicolon and everything after 
    // Lines will be of the form:
    // 'SUB    R3, R2'   
    const std::string whitespace {" \t"};
    const std::string endLine {";"};
    std::string::size_type start = input.find_first_not_of(whitespace);
    std::string::size_type end = input.find_first_of(endLine);  // Returns npos if not found
    // substring 2nd arg is length.   It will take all chars to end if npos.
    std::string trimString = input.substr(start, end == std::string::npos ? end : end-start); 
    return trimString;
}

//*************************************************************************************************
std::string Token(const std::string &input, std::string &token) {
    if (input.empty()) {
        token = "";
        return input;
    }
    try {
    // returns the first token
    const std::string delim {", \t"};
    std::string::size_type end = input.find_first_of(delim);
    // substring 2nd arg is length.   It will take all chars to end if npos.
    token = input.substr(0, end); 
    // If there are no more tokens we can't create a string from the end
    if (end != std::string::npos) {
        std::string s = input.substr(end);
        std::string::size_type start = s.find_first_not_of(delim);
        return s.substr(start);
    }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return std::string{};
}

//*************************************************************************************************
bool ParseOperation(const std::string &line, Operation &operation) {
   std::string token;  // First token on the line.  Either an opcode or a label
   std::string s = Token(line, token);
   if (token.empty()) {
      std::cout << "ParseOperation Failed empty opcode" << std::endl;
      return false; // Syntax Error
   }
   std::string label;
   Operation opcodeRule;
   if (!LookupOperation(token, opcodeRule)) {
       std::cout << "\tLABEL " << token;
       label = token;
       s = Token(s, token);
       if (!LookupOperation(token, opcodeRule)) {
          std::cout << "ERROR FINDING OPCODE TABLE ENTRY FOR: " << token << std::endl;
          return false;
       }
   }
   operation = opcodeRule;
   operation.label = label;
   // Now we have the table entry
   // Parse the REGISTERS, VALUES, LABELS
   if (opcodeRule.reg1 == REQUIRED) {
      s = Token(s, token);
      if (ParseRegister(token, operation.reg1)) {
        std::cout << "\tREG1 R" << operation.reg1;
      } else {
        return false;
      }
   }
   // Special handling for Register offset.   
   // Could have made a new table entry but to keep things simple we will do this.
   if (opcodeRule.extraLong && opcodeRule.reg2 == REQUIRED && opcodeRule.value == REQUIRED) {
      if (ParseRegisterOffset(s, operation.reg2, operation.value)) {
         std::cout << "\t REGOFF R" << std::to_string(operation.reg2) << ", " << std::hex << operation.value;
      }
   } else {
       if (opcodeRule.reg2 == REQUIRED) {
          s = Token(s, token);
          if (ParseRegister(token, operation.reg2)) {
            std::cout << "\tREG2 R" << operation.reg2;
          } else {
            return false;
          }
       } 
       if (opcodeRule.value == REQUIRED) {
          s = Token(s, token);
          if (ParseNumeric(token, operation.value)) {
            std::cout << "\tVALUE " << std::hex << operation.value;
          } else {
            return false;
          }
       }
       if (opcodeRule.label == HAS_LABEL) {
          if (!label.empty()) {
            std::cout << "JUMPING TO A JMP NOT PERMITTED" << std::endl;
            return false;
          }
          s = Token(s, token);
          operation.label = token;
          std::cout << "\tLABEL " << token;
       }
   }
   std::cout<< "\t["<< operation.opCode << "]" << std::endl;
   return true;
}

//*************************************************************************************************
bool LookupOperation (const std::string &token, Operation &opcodeRule) {
    // Now search our list to see if we find our token 
    for (const auto &op : OPCODE_LIST) {
        if (token == op.opCode) {
            opcodeRule = op;
            return true;
        }
    }
    return false;
}


//*************************************************************************************************
bool ParseRegister(const std::string &token, int &value) {
    if (token.length()) {
        std::string reg = token.substr(1); 
        try {
            value = std::stoi(reg.c_str());
            return true;
        }
        catch(...) {
            std::cerr << std::endl << "SYNTAX ERROR: REGISTER malformed " <<  token << std::endl;
        }

        return ParseNumeric(reg, value);
    }
    std::cerr << std::endl << "SYNTAX ERROR: Register malformed " <<  token << std::endl;
    return false;
}

//*************************************************************************************************
bool ParseNumeric(const std::string &token, int &value) {
    try {
        value = strtol(token.c_str(), nullptr, 16);
        return true;
    }
    catch(...) {
        std::cerr << std::endl << "SYNTAX ERROR: Numeric malformed " <<  token << std::endl;
    }
    return false;
}

//*************************************************************************************************
bool ParseRegisterOffset(const std::string &token, int &reg, int &value) {
    // Format:   M[R[reg], value]
    if (token.length() > 7 && token[0]== 'M' && token[1] == '[') {
        try {
            std::string s = token.substr(2);
            std::string regStr;
            s = Token(s, regStr);
            std::string valStr;
            Token(s, valStr);
            if(ParseRegister(regStr, reg) && ParseNumeric(valStr, value)) {
                return true;
            } else {
                std::cerr << std::endl << "SYNTAX ERROR: RegisterOffset malformed " <<  token << std::endl;
            }
        } catch(...) {
            std::cerr << std::endl << "SYNTAX ERROR: RegisterOffset malformed " <<  token << std::endl;
        }
    return false;
    } else {
        std::cerr << std::endl << "SYNTAX ERROR: RegisterOffset malformed " <<  token << std::endl;
    }
    return false;
}

//*************************************************************************************************
bool GenerateAssembly (const std::string &fileName, const Program &program) {
    bool success{true};
    std::cout << "Generating assembly..." << std::endl;
    std::ofstream outputFile;
    outputFile.open(fileName);
    if (!outputFile.is_open()) {
        std::cerr << "ERROR:  UNABLE TO OPEN OUTPUT FILE "<< fileName << std::endl;
    }
    constexpr uint16_t depth{1024};

    outputFile << "--Program Memory Initialization File" << std::endl;
    outputFile << "WIDTH = 16;" << std::endl;
    // outputFile << "DEPTH = " << depth << ";" << std::endl;
    outputFile << "ADDRESS_RADIX = HEX;" << std::endl;
    outputFile << "DATA_RADIX = BIN;"<< std::endl;
    outputFile << std::endl << "CONTENT BEGIN" << std::endl <<  std::endl;
    outputFile << "--A> : <OC><-Ri-><-Rj->" << std::endl;
    int addr{0};
    for ( const auto record : program) {
        constexpr int opcodeMask{0xF};
        constexpr int regMask{0x3F};
        uint16_t instr;
        std::bitset<16> bits;
        switch (record.operation.code) {
            case OPCODE_ADD:
            case OPCODE_COPY:
            case OPCODE_SWAP:
            case OPCODE_SUB:
            case OPCODE_AND:
            case OPCODE_OR:
                // These operations are OPCODE REG1 REG2
                instr = (record.operation.code & opcodeMask) << 12;
                instr |= (record.operation.reg1 & regMask) << 6;
                instr |= (record.operation.reg2 & regMask);
                bits = instr;
                outputFile<< ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                break;

            case OPCODE_LD:
            case OPCODE_ST:
                // These operations are OPCODE REG1 REG2 , VALUE
                instr = (record.operation.code & opcodeMask) << 12;
                instr |= (record.operation.reg1 & regMask) << 6;
                instr |= (record.operation.reg2 & regMask);
                bits = instr;
                outputFile << ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                bits = record.operation.value;
                outputFile << ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                break;
    
            case OPCODE_JMP:
                // These operations are OPCODE LABEL
                instr = (record.operation.code & opcodeMask) << 12;
                bits = instr;
                outputFile << ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                uint16_t offset;
                if (!GetJumpOffset(program,record.operation.label, record.addr, offset)) {
                    std::cerr << "ERROR LOOKING UP JMP LABEL " << record.operation.label << std::endl;
                    break;
                }
                bits = offset;
                outputFile << ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                break;

            case OPCODE_ADDC:
            case OPCODE_SUBC:
            case OPCODE_RRC:
            case OPCODE_SRA:
                // These operations are OPCODE REG1 VALUE
                instr = (record.operation.code & opcodeMask) << 12;
                instr |= (record.operation.reg1 & regMask) << 6;
                instr |= (record.operation.value & regMask);
                bits = instr;
                outputFile << ToString(addr) << " : " << bits << ";  %" << record.inputLine << "; %" << std::endl;
                addr++;
                break;
    
            case OPCODE_NOT:
                // These operations are OPCODE REG1
                instr = (record.operation.code & opcodeMask) << 12;
                instr |= (record.operation.reg1 & regMask) << 6;
                bits = instr;
                addr++;
                break;
        }
    }
    outputFile << "[ " << ToString(addr) << " .. " << ToString(depth-1) << " ] : 00000000; % Fill the remaining locations" << std::endl;
    outputFile << "END;" << std::endl;
    outputFile.close();
    return true;
}


//*************************************************************************************************
std::string ToString( uint16_t addr )
{
   char buf[10];
   snprintf(buf, sizeof(buf), "%04X", addr);
   std::string s(buf);
   return s;
}

//*************************************************************************************************
bool GetJumpOffset( const Program &program, const std::string &label, uint16_t fromAddr, uint16_t &offset )
{
    bool success{false};
    uint16_t toAddr;
    for (const auto record : program) {
        if (label == record.operation.label && record.addr != fromAddr) {
            toAddr = record.addr;
            offset = record.addr - fromAddr;
            printf ("JUMP %s FROM %04X TO %04X OFFSET=%04X\n", label.c_str(), fromAddr, record.addr, offset);
            success = true;
        }
    }
    return success;
}