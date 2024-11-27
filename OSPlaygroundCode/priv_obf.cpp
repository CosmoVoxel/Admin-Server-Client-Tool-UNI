#include <iostream>
#include <vector>


struct Data
{
	std::string name;
};

struct Info : public Data
{
	int age;
};

template <typename T>
class A
{
public:
	using data = T;
};

class B : public A<Info>
{
};


int main() {
	B::data info;
	info.age = 10;
	std::cout << info.age << std::endl;
}