#include <iostream>

void endianness() {
  // Test endianess
  int val = 0x30313233;
  char * ptr = (char *) &val;
  char * correct = (char *) "0123";
 
  // What a big endian system would print
  std::cout << "Big endian system: " << correct << std::endl;

  // What this system prints
  std::cout << "Your system: ";
  for (int i = 0; i < 4; i++) {
    std::cout << ptr[i];
  }
  std::cout << std::endl;

  // Print endianess
  if (*ptr == *correct) {
    std::cout << "Thus, your system is big endian." << std::endl;
  }
  else {
    std::cout << "Thus, your system is little endian." << std::endl;
  }
}

void otherfunction() {
  int a = 10;
  int b = a * 2;
  std::cout << a << " * 2 = " << b << std::endl;
}

int main() {
  // Statements should be printed to console
  std::cout << "This is a simple test of stdout." << std::endl;
  std::cerr << "This is a simple test of stderr." << std::endl;
  std::cerr << "Testing stderr again." << std::endl;
  std::cout << "Testing stdout again." << std::endl;

  // Arithmetic operations
  int a = 1;
  int b = 2;
  int c = 3;
  int d = a + b;

  // Comparison operations
  bool cd = c == d;

  // Print results of operations
  std::cout << "c is " << c << std::endl;
  std::cout << "d is " << d << std::endl;
  std::cout << "c = d? " << cd << std::endl;

  endianness(); 
  otherfunction();
}
