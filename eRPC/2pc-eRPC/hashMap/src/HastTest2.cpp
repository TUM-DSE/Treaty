#include <string>
#include <thread>
#include "HashMap.h"


class myInt {

	public:
		myInt() = delete;

		explicit myInt(int _int) {
			std::cout << "create with int " << _int << "\n";
			this->_int = _int;
			ptr = new int[_int];
		}

		~myInt() {
			std::cout << __PRETTY_FUNCTION__ << "\n";
			if (ptr != nullptr) 
				delete[] ptr;
		}

		myInt(const myInt& other) {
			std::cout << __PRETTY_FUNCTION__ << "\n";
			this->ptr = other.ptr;
			fprintf(stdout, "%p\n", this->ptr);
		}

		myInt& operator = (const myInt& other) {
			std::cout << __PRETTY_FUNCTION__ << "\n";
			this->ptr = other.ptr;
			fprintf(stdout, "%p\n", this->ptr);
			return *this;
		}

		myInt(myInt&& other) {
			assert(other.ptr);
			std::cout << __PRETTY_FUNCTION__ << "\n";
			_int = other._int;
			this->ptr = other.ptr;
			other._int = -1;
			other.ptr = nullptr;
			fprintf(stdout, "%p\n", this->ptr);
		}

		myInt& operator = (myInt&& other) {
			assert(other.ptr);
			std::cout << __PRETTY_FUNCTION__ << "\n";
			_int = other._int;
			this->ptr = other.ptr;
			other._int = -1;
			other.ptr = nullptr;
			fprintf(stdout, "%p\n", this->ptr);
			return *this;
		}

		int _int;
	private:
		int* ptr = nullptr;
};

int main() {

	//Multi threaded test with two threads
	CTSL::HashMap<std::string, std::shared_ptr<myInt>> integerMap;
	
	std::shared_ptr<myInt> a = std::make_shared<myInt>(25);
	std::shared_ptr<myInt> b = a;
	std::shared_ptr<myInt> c = std::make_shared<myInt>(26);
	integerMap.insert("dimitra", a);
	integerMap.insert("dimitra3", c);
	integerMap.insert("dimitra2", a);
	
	std::cout << a.use_count() << "\n";
	
	auto ptr = integerMap.find("dimitra");
	std::cout << ptr->get()->_int << " " << ptr->use_count() << "\n";
	
	ptr = integerMap.find("dimitra2");
	std::cout << ptr->get()->_int << " " << ptr->use_count() << "\n";

	std::shared_ptr<myInt> k = std::make_shared<myInt>(27);
	integerMap.insert("dimitra2", k);
	std::cout << k.use_count() << "\n";
	k.reset();

	// {
	ptr = integerMap.find("dimitra2");
	std::cout << ptr->get()->_int << " " << ptr->use_count() << "\n";
	// }
	//
	std::shared_ptr<myInt> question;
	integerMap.find("dimitra2", question);
	std::cout << question.get()->_int << " " << question.use_count() << "\n";
	question.get()->_int = 10;
	question.reset();
	
	std::cout << " ======= \n";
	std::shared_ptr<myInt>* _ptr = integerMap.find("dimitra2");
	std::cout << _ptr->get()->_int << " " << _ptr->use_count() << "\n";

	/*
	myInt c(20);
	integerMap.insert("key", c);
	*/
	/*
	integerMap.insert("dimitra", std::move(a));
	std::cout << a._int << "\n";
	*/

	return 0;

}

