#include "complex_store.h"
#include <sys/time.h>
#include <iostream>
#include <math.h>
#include "lz4.h"

#define CHECK_TIME 1
#define COLD_TEMP 80
#define INITIAL_TEMP 100.0
#define A 0.192
#define N 20.0
#define SIZE 300000

using namespace std;

int ComplexStore::get(const char* key, int key_size, char *value) {
	if (store.count(key)) {

        const char *source;

        if(store[key].compressed == true){
            //cout<<"decompressing..."<<endl;
            char decompressed[SIZE];
            int compressed_size = store[key].compressed_size;
	        int decompressed_size = LZ4_decompress_safe(store[key].value, decompressed,compressed_size,1024);
            //cout<<"compressed_size:"<<compressed_size<<endl;
            //cout<<"decompressed_size:"<<decompressed_size<<endl;
            delete [] store[key].value;
            store[key].value = new char[decompressed_size+1];
            decompressed[decompressed_size] = '\0';
            memcpy(store[key].value, decompressed, decompressed_size);
            store[key].value[decompressed_size] = '\0';
            store[key].compressed = false;
            hot_list.push_front(key);
        }
		source = store[key].value;
		int size = strlen(source);
		memcpy(value, source, size);		

        //calc current temperature
        int time = (this->getTime() - store[key].last_atime)/1000;
        double temp = this->calcTemp(store[key].temp, time);

        //modify new time and temperature
        store[key].temp = temp + N;
        store[key].last_atime = this->getTime();

        //if last_compressed_time = 0, initial it; otherwise now - last_modified_time > CHECK_TIME(10s), compress cold data
        if(last_compressed_time == 0){
            last_compressed_time = this->getTime();
        }else{
            int during_time = (this->getTime() - last_compressed_time)/1000;
            //cout<<"during_time:"<<during_time<<endl;
            if (during_time > CHECK_TIME){
                this->compressing();
                last_compressed_time = this->getTime();
            }
        }
		return size;
	} else {
		return -1;
	}
}

int ComplexStore::put(const char* key, int key_size, const char *value, int value_size) {
    info *p_info;
    p_info = &store[key];
    char compressed[SIZE]; 
    //cout<<"raw_value="<<value<<endl;
    int source_size = strlen(value);
    int compressed_size = LZ4_compress(value, compressed, source_size);
    
    if(p_info != NULL){
        (*p_info).compressed = true;
        (*p_info).temp = INITIAL_TEMP;
        (*p_info).last_atime = this -> getTime();
        delete [] (*p_info).value;
        (*p_info).value = new char[compressed_size];
        memcpy((*p_info).value,compressed,compressed_size);
        (*p_info).compressed_size = compressed_size;
    }else{
        info new_info;
        new_info.compressed = true;
        new_info.temp = INITIAL_TEMP;
        new_info.last_atime =  this->getTime();
        new_info.value = new char[compressed_size];
        memcpy(new_info.value, compressed, compressed_size);
        new_info.compressed_size = compressed_size;
	    store[key] = new_info;
    }
	return 0;
}

unsigned long long ComplexStore::getTime(void){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

double ComplexStore::calcTemp(double temp, int second){    
    return round(temp*exp(-A*second));
}

void ComplexStore::compressing(void){
   // cout<<"start compressing .... "<<endl;
    list<const char*>::iterator i= hot_list.begin();
    while(i != hot_list.end()){
       // cout<<"list:"<< *i << endl;
        //calc current temperature
        int time = (this->getTime() - store[*i].last_atime)/1000;
        double temp = this->calcTemp(store[*i].temp, time);
     //   cout<<"temp:"<<temp<<endl;
        if(temp < COLD_TEMP){
            store[*i].compressed = true;
            store[*i].last_atime = this->getTime();
            store[*i].temp = INITIAL_TEMP;
            char compressed[SIZE];
            char *value = store[*i].value;
            int source_size = strlen(value);
            int compressed_size = LZ4_compress(value, compressed, source_size);
         //   cout<<"compressed text: "<<compressed<<endl;
            delete [] store[*i].value;
            store[*i].value = new char[compressed_size];
            memcpy(store[*i].value, compressed, compressed_size);
            hot_list.erase(i++);
        }else{
            ++i;
        }
    }
    return;
}
