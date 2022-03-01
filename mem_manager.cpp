#include <thread>
#include <mutex>
#include <iostream>
#include <cassert>
#include <memory>
#include <cstring>
#include <string>


class Node
{
	public:
		Node* next;
		Node* prev;
		unsigned free;
		size_t	size;
		uint8_t*	memory;

		Node(void* memory, size_t size);

		void validate();
		void print();
};


Node::Node(void* _memory, size_t _size)
{
	assert(memory);
	assert(size > sizeof(Node));

	// TODO приведение типов
	memory = (uint8_t*)_memory;
	next = NULL;
	prev = NULL;
	free = 1;
	size = _size;
}

void Node::validate()
{
	assert(next == NULL || next->prev == this);
	assert(prev == NULL || prev->next == this);
	assert(size>0);
}

void Node::print()
{
	std::cout<<"address["<<std::hex<<this<<"] | ";
	std::cout<<"size["<<size<<"] | ";
	std::cout<<"free["<<free<<"]";
	std::cout<<std::endl;
}



class MemoryManager
{
	public:
		const int MINIMUM_FREE_BLOCK = 32;
		const int MINIMUM_HEAP_SIZE = 1024;

		static Node* linked_list;
		static Node* next_node;

		std::mutex lock;

		static size_t heap_size;


		//void* (*allocate)(size_t bytes);


		MemoryManager(void* memory, size_t size);

		Node* allocate_block(Node* p, size_t bytes);
		Node* merge_prev(Node* p);
		Node* merge_next(Node* p);
		void validate();
		void print();

		void* allocate(size_t bytes);
		void deallocate(void* memory);
};


MemoryManager::MemoryManager(void* memory, size_t size)
{
	assert(memory);
	assert(size > MINIMUM_HEAP_SIZE);

	Node* p = new Node(memory, size);

	heap_size = size;

	linked_list = p;

	next_node = NULL;

	//allocate = allocate_first_fit;
}

Node* MemoryManager::allocate_block(Node* p, size_t bytes)
{
	assert(p);
	assert(bytes > 0);

	size_t remaining = p->size - bytes;

	if (remaining >= MINIMUM_FREE_BLOCK)
	{
		Node* pnode = new Node(&p->memory[bytes], remaining);

		pnode->next = p->next;
		pnode->prev = p;

		if (pnode->next)
			pnode->next->prev = pnode;

		p->next = pnode;
		p->size = bytes;
	}

	p->free = 0;
	std::memset(p->memory, 0, p->size);

	return p;
}

Node* MemoryManager::merge_prev(Node* p)
{
	assert(p);

	p->prev->next = p->next;
	p->prev->size += p->size;

	if (p->next)
		p->next->prev = p->prev;

	return p->prev;
}

Node* MemoryManager::merge_next(Node* p)
{
	assert(p);

	p->size += p->next->size;

	if (p->next)
		p->next->prev = p;

	return p;
}

void MemoryManager::validate()
{
		lock.lock();
		Node* p = linked_list;
		size_t counter = 0;

		while(p)
		{
			p->validate();
			counter += p->size + sizeof(Node);
			p = p->next;
		}
		lock.unlock();
}

void MemoryManager::print()
{
	lock.lock();
	
	Node* p = linked_list;
	int i = 0;
	while(p) 
	{
		std::cout<<"node["<<i<<"] | ";
		p->print();
		p = p->next;
	}

	lock.unlock();
}


void* MemoryManager::allocate(size_t bytes)
{
	lock.lock();

	assert(bytes > 0);

	Node* p = linked_list;

	assert(p);

	while(p)
	{
		if (p->free && p->size >= bytes)
		{
			allocate_block(p, bytes);
			lock.unlock();
			return p->memory;
		}

		p = p->next;
	}

	lock.unlock();
	return NULL;
}
