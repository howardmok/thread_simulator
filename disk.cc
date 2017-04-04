#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <cmath>
#include <algorithm>
#include "thread.h"

using namespace std;

int MAX_DISK_QUEUE;
int NUM_DISK;
int NUM_QUEUE = 0;

intptr_t trackNum;

bool in_queue;
bool first = true;

char **g_argv;

string line;

vector<pair<int, int>> files;
vector<int> q_threads;

mutex request_lock;
cv request;
cv serve;

void service(void *track_num)
{   
    if (first) {
        trackNum = (intptr_t) track_num;
        first = false;
    }

    request_lock.lock();

    while (1) {

        int chosen_one;
        int chosen_two;
        int index;
        int trackDif = 1000;

        while (NUM_QUEUE != MAX_DISK_QUEUE){
            serve.wait(request_lock);
        }

        if (NUM_QUEUE == 0 && NUM_DISK == 0){
            break;
        }

        for (int i = 0; i < files.size(); i++){
            int tempDif = abs(files[i].second - trackNum);
            if (tempDif < trackDif) {
                trackDif = tempDif;
                chosen_one = files[i].first;
                chosen_two = files[i].second;
                index = i;
            }
        }

        trackNum = chosen_two;
        cout << "service requester " << chosen_one << " track " << chosen_two << endl;

        for (int i = 0; i < q_threads.size(); i++) {
            if (q_threads[i] == chosen_one) {
                q_threads.erase(q_threads.begin() + i);
            }
        }

        files.erase(files.begin() + index);

        NUM_QUEUE--;

        request.broadcast();

    }

    request_lock.unlock();

    return;

}

void requester(void *file_num)
{

    intptr_t requesterNum = (intptr_t) file_num;
    ifstream file(g_argv[requesterNum+2]);

    request_lock.lock();

    while (1) {

        while (NUM_QUEUE == MAX_DISK_QUEUE || (find(q_threads.begin(), q_threads.end(), requesterNum) != q_threads.end())) {
            request.wait(request_lock);
        }

        for (int i = 0; i < files.size(); i++) {
            if (files[i].first == requesterNum) {
                in_queue = true;
                break;
            }
            else {
                in_queue = false;
            }
        }

        if (files.size() == 0) {
            in_queue = false;
        }

        if (!in_queue) {

            getline(file, line);

            if (line.empty()){
                break;
            }

            cout << "requester " << requesterNum << " track " << line << endl;
            files.push_back(make_pair(requesterNum, atoi(line.c_str())));
            q_threads.push_back(requesterNum);
            NUM_QUEUE++;

            serve.signal();

        }

    }

    NUM_DISK--;

    if (NUM_DISK < MAX_DISK_QUEUE) {
        MAX_DISK_QUEUE--;
    }

    serve.signal();

    request_lock.unlock();

}

void create_threads(void *a) {

    for (int i = 0; i < NUM_DISK; i++) {
        thread t1 ((thread_startfunc_t) requester, (void *) i);
    }

    thread t2 ((thread_startfunc_t) service, (void *) 0);

}

int main(int argc, char* argv[])
{

    MAX_DISK_QUEUE = atoi(argv[1]);
    NUM_DISK = argc - 2;
    if (NUM_DISK < MAX_DISK_QUEUE) {
        MAX_DISK_QUEUE = NUM_DISK;
    }
    g_argv = argv;

    cpu::boot((thread_startfunc_t) create_threads, (void *) 0, 0);

}