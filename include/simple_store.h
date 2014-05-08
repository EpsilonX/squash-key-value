#include <functional>

#include <tr1/unordered_map>
#define hash_map std::tr1::unordered_map
#include "store.h"

using namespace std;

class SimpleStore : public Store {
public:
	/**
	 * @return the size of the value, or -1 if the value it not available
	 */
	virtual int get(const char* key, char *value);


	virtual int put(const char* key, const char *value) {
		return put(key, value, strlen(value));
	}

	/**
	 * @return 0 if put is success, or -1 if not
	 */
	virtual int put(const char* key, const char *value, int len);

private:
    typedef struct {
        bool compressed;
        int temp;
        int last_atime;
        const char *value;
    } info;

    hash_map<const char*, info> store;
};
