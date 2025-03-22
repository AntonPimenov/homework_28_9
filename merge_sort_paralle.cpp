#include <iostream>
#include <vector>
#include <future>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib> // для srand и rand

// Порог для последовательной сортировки
const int SEQUENTIAL_THRESHOLD = 1000;

// Счетчик активных потоков
std::atomic<int> active_threads(0);

// Функция для слияния двух отсортированных подмассивов
void merge(std::vector<int>& arr, int l, int m, int r) {
    int nl = m - l + 1;
    int nr = r - m;

    std::vector<int> left(arr.begin() + l, arr.begin() + m + 1);
    std::vector<int> right(arr.begin() + m + 1, arr.begin() + r + 1);

    int i = 0, j = 0, k = l;
    while (i < nl && j < nr) {
        if (left[i] <= right[j]) {
            arr[k] = left[i];
            i++;
        } else {
            arr[k] = right[j];
            j++;
        }
        k++;
    }

    while (i < nl) {
        arr[k] = left[i];
        i++;
        k++;
    }

    while (j < nr) {
        arr[k] = right[j];
        j++;
        k++;
    }
}

// Рекурсивная функция сортировки слиянием (многопоточная версия)
void merge_sort_mt(std::vector<int>& arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;

        // Если размер массива больше порога и количество активных потоков не превышено,
        // запускаем параллельную сортировку
        if ((r - l + 1) > SEQUENTIAL_THRESHOLD && active_threads.load() < std::thread::hardware_concurrency()) {
            active_threads.fetch_add(2); // Увеличиваем счетчик активных потоков

            // Запускаем сортировку левой части в отдельном потоке
            auto left_future = std::async(std::launch::async, [&arr, l, m]() {
                merge_sort_mt(arr, l, m);
            });

            // Запускаем сортировку правой части в отдельном потоке
            auto right_future = std::async(std::launch::async, [&arr, m , r]() {
                merge_sort_mt(arr, m + 1, r);
            });

            // Ожидаем завершения работы потоков
            left_future.get();
            right_future.get();

            active_threads.fetch_sub(2); // Уменьшаем счетчик активных потоков
        } else {
            // Если размер массива меньше порога или достигнут лимит потоков,
            // выполняем сортировку последовательно
            merge_sort_mt(arr, l, m);
            merge_sort_mt(arr, m + 1, r);
        }

        // Слияние отсортированных частей
        merge(arr, l, m, r);
    }
}

// Рекурсивная функция сортировки слиянием (однопоточная версия)
void merge_sort_st(std::vector<int>& arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;

        // Рекурсивно сортируем левую и правую части
        merge_sort_st(arr, l, m);
        merge_sort_st(arr, m + 1, r);

        // Слияние отсортированных частей
        merge(arr, l, m, r);
    }
}

// Генерация случайного массива
std::vector<int> generate_random_array(size_t size) {
    std::vector<int> arr(size);
    for (size_t i = 0; i < size; ++i) {
        arr[i] = rand() % 10000; // случайные числа от 0 до 9999
    }
    return arr;
}

int main() {
    // Инициализация генератора случайных чисел
    srand(static_cast<unsigned int>(time(0)));

    // Размер массива
    const size_t array_size = 1000000; // 1 миллион элементов
    std::vector<int> arr = generate_random_array(array_size);

    // Копия массива для однопоточной версии
    std::vector<int> arr_st = arr;

    // Замер времени для многопоточной версии
    auto start_mt = std::chrono::high_resolution_clock::now();
    merge_sort_mt(arr, 0, arr.size() - 1);
    auto end_mt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_mt = end_mt - start_mt;

    // Замер времени для однопоточной версии
    auto start_st = std::chrono::high_resolution_clock::now();
    merge_sort_st(arr_st, 0, arr_st.size() - 1);
    auto end_st = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_st = end_st - start_st;

    // Вывод результатов
    std::cout << "Многопоточная сортировка выполнена за: " << elapsed_mt.count() << " секунд\n";
    std::cout << "Однопоточная сортировка выполнена за: " << elapsed_st.count() << " секунд\n";

    return 0;
}