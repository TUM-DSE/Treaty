#include <stdlib.h>
#include <memory>

int main() {
	int* _x;

	std::unique_ptr<int[]> x(new int[10]);
	_x = x.get();

	for (int i = 0; i < 11; i++)
		_x[i] = i;

	return 1;
}
