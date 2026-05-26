#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

// Structure to store a ratio
struct Ratio {
    float value;
    int numerator;
    int denominator;
};

// Generate all 3-limit ratios within [minRatio, maxRatio]
std::vector<Ratio> Generate3LimitRatios(float minRatio = 0.5f, float maxRatio = 2.0f) {
    std::vector<Ratio> results;

    // Search powers from -6 to +6 for both primes
    for (int n = -6; n <= 6; ++n) {
        for (int m = -6; m <= 6; ++m) {
            float ratio = powf(2.0f, n) * powf(3.0f, m);
            if (ratio >= minRatio && ratio <= maxRatio) {
                // Reduce to a fraction approximation
                int num = static_cast<int>(roundf(ratio * 10000));
                int den = 10000;

                // Reduce fraction
                int a = num, b = den;
                while (b != 0) {
                    int t = b;
                    b = a % b;
                    a = t;
                }
                int gcd = a;

                results.push_back({ ratio, num / gcd, den / gcd });
            }
        }
    }

    // Sort by ratio value
    std::sort(results.begin(), results.end(), [](const Ratio& a, const Ratio& b) {
        return a.value < b.value;
    });

    return results;
}

int main() {
    auto ratios = Generate3LimitRatios(0.5f, 2.0f);
    for (const auto& r : ratios) {
        std::cout << r.numerator << ":" << r.denominator << " (" << r.value << ")" << std::endl;
    }
    return 0;
}
