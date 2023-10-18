// Assembler written by Cody March
/*
int main() 
openfile()      Open assembly file
filetoarray()   Read in assembly file, storing each non blank line as an index in an array of strings (or a vector would be great)
step1()         Break apart array of assembly strings into single IW instruction with gaps for double IW instructions
step2()         Translate array of machine code to binary, inserting addresses for jump to locations
arraytomif()    Print binary machine code array to mif file.

This represents the flow the program should go through
*/

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

// Fields filled in with REQUIRED/NONE:
const int REQUIRED{-1};
const int NONE{-2};
const std::string HAS_LABEL{"LABEL"};
const std::string NO_LABEL{""};

using Program = std::vector<Operation>;

// Array of strings containing all the Op Codes in their respective order. IN and OUT will become VADD and VSUB later.
// std::string Ops[16] = {"LD", "ST", "CPY", "SWAP", "JMP", "ADD", "SUB", "ADDC", "SUBC", "NOT", "AND", "OR", "SRA", "RRC", "IN", "OUT"};
const Program OPCODE_LIST = {
    // Opcode       code    // ExtraLong     R1          R2            Value   Label
    { "LD",         0x00,   true,           REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    { "ST",         0x01,   true,           REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    { "CPY",        0x02,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SWAP",       0x03,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "JMP",        0x04,   true,           REQUIRED,   REQUIRED,   REQUIRED,  HAS_LABEL },
    { "ADD",        0x05,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SUB",        0x06,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "ADDC",       0x07,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SUBC",       0x08,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "NOT",        0x09,   false,          REQUIRED,   NONE,       NONE,      NO_LABEL  },
    { "AND",        0x0A,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "OR",         0x0B,   false,          REQUIRED,   REQUIRED,   NONE,      NO_LABEL  },
    { "SRA",        0x0C,   false,          REQUIRED,   NONE,       REQUIRED,  NO_LABEL  },
    { "RRC",        0x0D,   false,          REQUIRED,   NONE,       REQUIRED,  NO_LABEL  }
    // { "IN",         0x0E,   false,          REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
    // { "OUT",        0x0F,   false,          REQUIRED,   REQUIRED,   REQUIRED,  NO_LABEL  },
};

// FUNCTION DECLARATIONS

// Parse the file
using FileVector = std::vector<std::string>;
FileVector ParseFile (std::string &fileName);

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
bool GenerateAssembly (const Program &program);

// Search our OPCODE Table for token.  If we find it, opcodeRule points the the row in the table and we return true.
// Otherwise return false
bool LookupOperation (const std::string &token, Operation &opcodeRule);

// Load , Store, Jump are doubles

//*************************************************************************************************
int main(int argc, char** argv) {
    bool success{true};
    std::string fileName{"dxp_dcs.txt"};
    FileVector fileVector = ParseFile(fileName);

    Program program;
    for ( const auto line : fileVector) {
        std::cout << "PARSE '" << line  << "'" << std::endl;
       Operation operation{};
       if (ParseOperation(line, operation)) {
           program.push_back(operation);
       } else {
           std::cout << "SYNTAX ERROR: '" << line << "'" << std::endl;
           success = false;
       }
    }

    if (success) {
        std::cout << "Processing Complete...Generating Assembly" << std::endl;
        success = GenerateAssembly(program);

        if (!success) {
            std::cout << "FAILED TO GENERATE ASSEMBLY" << std::endl;
        }
    }

    return success ? 0 : -1;
}


//*************************************************************************************************
FileVector ParseFile (std::string &fileName) {
    FileVector fileVector;
    std::ifstream myFile;

    const std::string codeStart{".code"};
    const std::string codeEnd{".endcode"};

    myFile.open(fileName);
    
    if (myFile.is_open()) {
        // This loop will run until it reaches the end of the file
        bool codeBlock{false};
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
    // returns the first token
    const std::string delim {", \t"};
    std::string::size_type end = input.find_first_of(delim);
    // substring 2nd arg is length.   It will take all chars to end if npos.
    token = input.substr(0, end); 
    std::string s = input.substr(end);
    printf ("%s trim '%s'\n", __func__, s.c_str());
    // Now remove the extra white space
    std::string::size_type start = s.find_first_not_of(delim);
    printf ("%s returns '%s'\n", __func__, s.substr(start).c_str());
    return s.substr(start);
}

//*************************************************************************************************
bool ParseOperation(const std::string &line, Operation &operation) {
   std::string firstToken;  // First token on the line.  Either an opcode or a label
   std::string s = Token(line, firstToken);
   if (firstToken.empty()) {
      std::cout << "ParseOperation Failed empty opcode" << std::endl;
      return false; // Syntax Error
   }
   
   std::string label;
   Operation opcodeRule;
   if (LookupOperation(firstToken, opcodeRule)) {
       std::cout << "...Table Entry Found" << std::endl;

   } else {
       std::cout << "...label found '" << firstToken << "'" << std::endl;
       label = firstToken;
       if (LookupOperation(firstToken, opcodeRule)) {
          std::cout << "...Table Entry Found" << std::endl;
       } else {
          std::cout << "ERROR FINDING OPCODE TABLE ENTRY FOR: " << firstToken << std::endl;
          return false;
       }
   }
   operation = opcodeRule;
   operation.label = label;
   // Now we have the table entry
   // Parse the REGISTERS, VALUES, LABELS
   if (opcodeRule.reg1 == REQUIRED) {
      s = Token(s, firstToken);
      std::cout << "... REG1 " << firstToken << std::endl;
   } else if (opcodeRule.reg2 == REQUIRED) {
      s = Token(s, firstToken);
      std::cout << "... REG2 " << firstToken << std::endl;      
   } else if (opcodeRule.value == REQUIRED) {
      s = Token(s, firstToken);
      std::cout << "... VALUE " << firstToken << std::endl;      
   } else if (opcodeRule.label == HAS_LABEL) {
      if (!label.empty()) {
        std::cout << "JUMPING TO A JMP NOT PERMITTED" << std::endl;
        return false;
      }
      s = Token(s, firstToken);
      std::cout << "... LABEL " << firstToken << std::endl;            

   }
   std::cout<< "Processing... ["<< operation.opCode << "]" << std::endl;
   return true;
}


//*************************************************************************************************
bool GenerateAssembly (const Program &program) {
    std::cout << "Generating assembly..." << std::endl;
    return false;
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

