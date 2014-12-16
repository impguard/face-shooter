#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;

void test(float x, float y, float z) {
    stringstream ss;
    ss << "python post.py " << x << " " << y << " " << z;

    system(ss.str().c_str());
}

int main(int argc, char** argv) {
    if (argc < 4) {
        cout << "Pass 3 floats please." << endl;
        exit(1);
    }

    float x = atof(argv[1]);
    float y = atof(argv[2]);
    float z = atof(argv[3]);

    test(x, y, z);

    cout << "Data sent" << endl;
}