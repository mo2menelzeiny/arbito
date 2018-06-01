#include <iostream>
#include <chrono>
#include <libtrading/proto/fix_session.h>

void benchmark() {
    std::string str("8=FIX.4.4 9=75 35=A 49=LMXBD 56=AhmedDEMO 34=1 52=20180531-04:52:25.328 98=0 108=30 141=Y 10=173");
    long count = 0, sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }
    std::cout << "wasted: " << sum / count << std::endl;
    count = 0;
    sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        // put test case here
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }

    std::cout << "total: " << sum / count << std::endl;

}

int something() {


    return EXIT_SUCCESS;
}