#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>

using namespace std;

class File_Parser {
    vector<string> words_buff; //буффер, общий для всех потоков
    int threads_num;
    int key_word_count;
    mutex file_locker, parser_locker;  //parser_locker будет блокировать поток, вызвавший count_key_words до того момента, пока не закончится метод
    string key;
    bool file_ended;

    function<void(ifstream*, int)> add_words = [this](ifstream* inp, int id){
        string cur_word;

        while(true) {
            //критическая секция
            //file_locker.lock();
            lock_guard<mutex> file_l(file_locker);
            if(!inp->good() || file_ended) {
                file_ended = true; //для других потоков
                break;
            }
            *inp >> cur_word;
            words_buff.emplace_back(cur_word);
            cout << "Thread " << id << " just added" << endl;
        }
    };

    function<void(int, int)> count_key_word_in_section = [this](int from, int len) {
        //критическая секция
        //file_locker.lock(); - секции не пересекаются, мьютекс не нужен?
        for(int i = from; i < from+len; i++) {
            if(words_buff[i] == key) {
                key_word_count++;
            }
        }
        //file_locker.unlock();
    };

    void clear() {  //очистка данных после каждого парсинга файла
        key = "";
        key_word_count = 0;
        file_ended = false;
        words_buff.clear();
    }

public:

    File_Parser(): threads_num(0), key(""), key_word_count(0), file_ended(false){
        words_buff = vector<string>(0); //move assignment operator
    }
    ~File_Parser() {}

    int count_key_words(ifstream* inp, const string& key_word, int threads) {
        parser_locker.lock();
        key = key_word;
        threads_num = threads;
        vector<thread> input_threads(0);

        //Считывание и заполнение массива слов ________________

        for(int i = 0; i < threads_num; i++) {
            input_threads.emplace_back(add_words, inp, i);
            //cout << "Started " << i << endl;
        }

        for(int i = 0; i < threads_num; i++) {
            input_threads[i].join();
        }

        //Подсчет вхождений ключевого слова
        int words_num = words_buff.size(),
            last_thread_sec_len = words_num % threads,
            section_len = words_num / threads;
        last_thread_sec_len = (last_thread_sec_len == 0 ? section_len : last_thread_sec_len);
        vector<thread> counter_threads(0);

        for(int i = 0; i < threads-1; i++) {
            counter_threads.emplace_back(count_key_word_in_section, i*section_len, section_len);
        }
        counter_threads.emplace_back(count_key_word_in_section, (threads-1)*section_len, section_len+last_thread_sec_len);

        for(int i = 0; i < threads; i++) {
            counter_threads[i].join();
        }

        int res = key_word_count;
        this->clear();  //чтобы другой поток смог воспользоваться этим объектом

        parser_locker.unlock();
        return res;
    }

};

int main() {

    int threads_num;
    string key_word;
    ifstream inp("D:\\Programming\\ClionProjects\\Thread_test\\input.txt");
    ofstream outp("D:\\Programming\\ClionProjects\\Thread_test\\output.txt");

    inp >> threads_num;
    inp >> key_word;

    File_Parser parser;
    outp << parser.count_key_words(&inp, key_word, threads_num);
}