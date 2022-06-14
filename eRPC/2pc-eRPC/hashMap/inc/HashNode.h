#ifndef HASH_NODE_H_
#define HASH_NODE_H_

#include <shared_mutex>
#include <cassert>
namespace CTSL  //Concurrent Thread Safe Library
{
	// Class representing a templatized hash node
	template <typename K, typename V>
		class HashNode
		{
			public:
				HashNode() : next(nullptr) {

				}

				HashNode(const K& key_, V&& value_, int am = 1) : next(nullptr), key(key_), value(std::move(value_)) {
				}

				// HashNode(const K& key_, const V& value_) : next(nullptr), key(key_), value(value_) {
				HashNode(const K& key_, const V& value_) {
					// std::cout << __PRETTY_FUNCTION__ << "\n";
					next = nullptr;
					key = key_; 
					value = value_;
				}


				~HashNode() {
					next = nullptr;
				}

				const K& getKey() const {
					return key;
				}

				void setValue(V&& value_) {
					// std::cout << __PRETTY_FUNCTION__ << "\n";
					value = std::move(value_);
				}

				void setValue(const V& value_) {
					// std::cout << __PRETTY_FUNCTION__ << "\n";
					value = value_;
				}

				const V& getValue() const {
					return value;
				}

				V& _getValue() {
					return value;
				}

				V* getValuePtr() { 
					return &value; 
				}

				HashNode *next; // Pointer to the next node in the same bucket
			private:
				K key;   // the hash key
				V value; // the value corresponding to the key
		};


		//Class representing a hash bucket. The bucket is implemented as a singly linked list.
		//A bucket is always constructed with a dummy head node
		template <typename K, typename V>
			class HashBucket {
				public:
					HashBucket() : head(nullptr) {}

					~HashBucket() { // delete the bucket 
						clear();
					}   

					/**
					 * Function to find an entry in the bucket matching the key.
					 * If key is found, the corresponding value is copied into the parameter "value" and function returns true.
					 * If key is not found, function returns false.
					 */
					bool find(const K &key, V &value) const {
						// std::cout << __PRETTY_FUNCTION__ << "\n";
						// A shared mutex is used to enable mutiple concurrent reads
						std::shared_lock<std::shared_timed_mutex> lock(mutex_); 
						HashNode<K, V> * node = head;

						while (node != nullptr)
						{
							if (node->getKey() == key)
							{
								value = node->getValue();
								return true;
							}
							node = node->next;
						}
						return false;
					}

					V* find(const K &key) {
						// A shared mutex is used to enable mutiple concurrent reads
						std::shared_lock<std::shared_timed_mutex> lock(mutex_); 
						HashNode<K, V> * node = head;

						while (node != nullptr) {
							// std::cout << key << " " << "\n";
							if (node->getKey() == key) {
								V* ptr = node->getValuePtr();
								return ptr;
							}
							node = node->next;
						}
						// std::cout << "return nullptr\n";
						return nullptr;
					}

					V& find_ref(const K& key) {
						// A shared mutex is used to enable mutiple concurrent reads
						std::shared_lock<std::shared_timed_mutex> lock(mutex_); 
						HashNode<K, V> * node = head;

						while (node != nullptr) {
							if (node->getKey() == key) {
								return node->_getValue();
							}
							node = node->next;
						}
						std::cout << __PRETTY_FUNCTION__ << " SHOULD BREAK HERE\n";
						assert(false);
						return head->_getValue(); // this is for compiler warnings
					}


					// Function to insert into the bucket
					// If key already exists, update the value, else insert a new node in the bucket with the <key, value> pair
					void insert(const K &key, V&& value) {
						// std::cout << __PRETTY_FUNCTION__ << " "  << key << " " << "\n";

						// Exclusive lock to enable single write in the bucket
						std::unique_lock<std::shared_timed_mutex> lock(mutex_);
						HashNode<K, V> * prev = nullptr;
						HashNode<K, V> * node = head;

						while (node != nullptr && node->getKey() != key) {
							prev = node;
							node = node->next;
						}

						if (nullptr == node) { // New entry, create a node and add to bucket
							if (nullptr == head) {
								head = new HashNode<K, V>(key, std::move(value), 1);
							}
							else {
								prev->next = new HashNode<K, V>(key, std::move(value), 1);                 
							}
						}
						else {
							node->setValue(std::move(value)); // Key found in bucket, update the value
						}

					}


					// Function to insert into the bucket
					// If key already exists, update the value, else insert a new node in the bucket with the <key, value> pair
					void insert(const K &key, const V & value) {
						// Exclusive lock to enable single write in the bucket
						// std::cout << __PRETTY_FUNCTION__ << " "  << key << " " << " \n";
						std::unique_lock<std::shared_timed_mutex> lock(mutex_);
						HashNode<K, V> * prev = nullptr;
						HashNode<K, V> * node = head;

						while (node != nullptr && node->getKey() != key) {
							prev = node;
							node = node->next;
						}

						if (nullptr == node) { // New entry, create a node and add to bucket
							if (nullptr == head) {
								head = new HashNode<K, V>(key, value);
							}
							else {
								prev->next = new HashNode<K, V>(key, value);                 
							}
						}
						else {
							node->setValue(value); // Key found in bucket, update the value
						}

					}


					// Function to remove an entry from the bucket, if found
					void erase(const K &key) {
						// Exclusive lock to enable single write in the bucket
						std::unique_lock<std::shared_timed_mutex> lock(mutex_);
						HashNode<K, V> *prev  = nullptr;
						HashNode<K, V> * node = head;

						while (node != nullptr && node->getKey() != key)
						{
							prev = node;
							node = node->next;
						}

						if (nullptr == node) // Key not found, nothing to be done
						{
							return;
						}
						else  // Remove the node from the bucket
						{
							if(head == node)
							{
								head = node->next;
							}
							else
							{
								prev->next = node->next; 
							}
							delete node; // Free up the memory
						}
					}

					// Function to clear the bucket
					void clear() {
						// Exclusive lock to enable single write in the bucket
						std::unique_lock<std::shared_timed_mutex> lock(mutex_);
						HashNode<K, V> * prev = nullptr;
						HashNode<K, V> * node = head;
						while(node != nullptr)
						{
							prev = node;
							node = node->next;
							delete prev;
						}
						head = nullptr;
					}

				private:
					HashNode<K, V> * head; // The head node of the bucket
					mutable std::shared_timed_mutex mutex_; // The mutex for this bucket
			};
		}

#endif

