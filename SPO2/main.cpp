#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <conio.h>
#include <bitset>
#include <stdio.h>
#include <vector>
#include <math.h>
#include "test1.cpp"

unsigned const int size_page = 20;//для 4 кБ=4096
unsigned const char amount_pages=4;// max=127;
unsigned const int _pool_size = amount_pages * size_page;// тут как 128 - т.е. общее колво, а нумерация будет идти 0-32767,например;

unsigned char pool[_pool_size];
using namespace std;


struct page_header
{
	unsigned int state : 2, pointer_1st_empty_block : 12, amount_empty_block : 11, size_class:7;
};



struct pointer_block_class
{
	vector< page_header * > table_class_block;
	int block_size;
};


vector< page_header * > table_descriptors_pages;
vector< page_header * > table_descriptors_empty_pages;
vector< pointer_block_class > table_class_block;
//vector<vector<el_with_the_same_class>> table_class_block;

int search_place_in_array_for_block(void *el)//поиск для освобождения блока его места в массиве по ссылке на него
{
	int i;
	for ( i = 0; i < _pool_size; i++){
		if (&pool[i] == el){
			break;
		}
	}
	return i;
}

int search_page_of_block(void *el){//поиск страницы
	int i;
	for (i = 0; i <= table_descriptors_pages.size(); i++){
		if (el < table_descriptors_pages[i]){
			break;
		}
	}
	return i;
}

int search_place(int size){//позиция в таблице классов вектора с нужным классом
	int i;
	for (i = 0; i < table_class_block.size(); i++){
		if (table_class_block[i].block_size == size){
			break;
		}
	}
	return i;
}

int search_class_by_header(page_header* header){
	int i;
	int j;
	bool flag = false;
	for (i = 0; i < table_class_block.size(); i++){
		for (j = 0; j < table_class_block[i].table_class_block.size(); j++){
			if (table_class_block[i].table_class_block[j] == header){
				flag = true;
				break;
			}
		}
		if (flag == true){ break; }
	}
	return table_class_block[i].block_size;
}

int search_position_in_empty(page_header *h){//поиск, куда вставлять в вектор пустых очередной эл-т, чтобы сохранился порядок
	int i;
	for (i = 0; i < table_descriptors_empty_pages.size(); i++){
		if (table_descriptors_empty_pages[i] > h){
			break;
		}
	}
	return i;
}


void delete_from_class_table(page_header* header){
	int i;
	int j;
	bool flag = false;
	for (i = 0; i < table_class_block.size(); i++){
		for (j = 0; j < table_class_block[i].table_class_block.size(); j++){
			if (table_class_block[i].table_class_block[j] == header){
				flag = true;
				break;
			}
		}
		if (flag == true){ break; }
	}
	table_class_block[i].table_class_block.erase(table_class_block[i].table_class_block.begin() + j);
}


void descriptor_writing(page_header *h, unsigned int state, unsigned int pointer_1st_empty_block, unsigned int amount_empty_block, unsigned char size_class)
{
	(*h).state = state;
	(*h).pointer_1st_empty_block = pointer_1st_empty_block;
	(*h).amount_empty_block = amount_empty_block;
	(*h).size_class = size_class;
	//x.push_back(vector<int>(1));

}

void fill_memory(unsigned char*p, int n, char c){
	for (int i = 0; i < n; i++)
	{
		*(p + i) = c;
	}
}

int   size_align(int  size)
{
	int t = (log(size)) / (log(2));
	
	if (pow(2, t) != size) {
		t++;
	}
	return pow(2,t);
}

void *mem_alloc(size_t size)
{
	// как выделять многостраничный блок?
	//проверять, есть ли столько памяти?
	cout << "Allocing memory (" << size << ") byte"<<endl;
	unsigned int requested_size = size_align(size);
	//while (size >>= 1) requested_size <<= 1;
	if (size> size_page-sizeof(page_header)){//если нужно выделять несколько страниц
		unsigned int requested_num_page = ceil((double)size / (double)(size_page - sizeof(page_header)));
		if (table_descriptors_empty_pages.size() < requested_num_page){
			return NULL;
		}
		else{//если хватает места на столько страниц
			unsigned char *hed_empty = (unsigned char *)table_descriptors_empty_pages[0] - size_page + sizeof(page_header);
			for (int k = 0; k < requested_num_page; k++){
				table_descriptors_empty_pages[0]->state = 2;
				table_descriptors_empty_pages[0]->amount_empty_block = 0;
				table_descriptors_empty_pages[0]->pointer_1st_empty_block = 0;
				table_descriptors_empty_pages[0]->size_class = 0;
				

				hed_empty = (unsigned char *)table_descriptors_empty_pages[0] - size_page + sizeof(page_header);
				fill_memory(hed_empty, size_page - sizeof(page_header),'*');

				table_descriptors_empty_pages.erase(table_descriptors_empty_pages.begin());
				
			}
			return (unsigned char *)(hed_empty - size_page*(requested_num_page-1));
		}

	}
	else{//если это не многостраничный блок
		unsigned int requested_size = size_align(size);
		bool find_flag = false;
		int i = search_place(requested_size);
		if (table_class_block[search_place(requested_size)].table_class_block.size() != 0)
		{
			find_flag = true;
		}

		if (find_flag){//если уэе есть страница под такой класс
			unsigned short int adress_1st = table_class_block[i].table_class_block[0]->pointer_1st_empty_block;
			unsigned char *hed;
			if (table_class_block[i].table_class_block[0]->amount_empty_block != 1){
				unsigned int amount = (table_class_block[i].table_class_block[0]->amount_empty_block) - 1;
				unsigned char* start;//определяем новую первую свободную страницу
				start = (unsigned char*)table_class_block[i].table_class_block[0] - size_page + sizeof(page_header) + table_class_block[i].table_class_block[0]->pointer_1st_empty_block;
				descriptor_writing((table_class_block[i].table_class_block[0]), 1, *(unsigned short*)start, amount, table_class_block[i].table_class_block[0]->size_class);
				hed = (unsigned char *)table_class_block[i].table_class_block[0] - size_page + sizeof(page_header) + adress_1st;
			}
			else{//это был последний блок	
				descriptor_writing((table_class_block[i].table_class_block[0]), 1, 0, 0, table_class_block[i].table_class_block[0]->size_class);
				hed = (unsigned char *)table_class_block[i].table_class_block[0] - size_page + sizeof(page_header) + adress_1st;
				table_class_block[i].table_class_block.erase(table_class_block[i].table_class_block.begin());
			}
			//unsigned char *hed = (unsigned char *)table_class_block[i].table_class_block[0] - size_page + sizeof(page_header) + adress_1st;
			fill_memory(hed, requested_size, '*');
			return (hed);
		}
		else{//выделение новой страницы
			if (table_descriptors_empty_pages.size() != 0){
				unsigned char *hed_empty;
				unsigned int amount_empty_block = (size_page - sizeof(page_header)) / requested_size;
				unsigned short int size_class = 0;
				unsigned short int pointer_1st_empty_block = requested_size;
				if (size_page - sizeof(page_header) != requested_size){
					descriptor_writing((table_descriptors_empty_pages[0]), 1, pointer_1st_empty_block, (amount_empty_block - 1), ((log(requested_size)) / (log(2))));
					for (int i2 = 0; i2 < ((int)size_page - 4 - 2 * (int)requested_size); i2 = i2 + requested_size){
						*((unsigned char*)(table_descriptors_empty_pages[0]) - size_page + 4 + requested_size + i2) = (unsigned short int)(i2 + 2 * requested_size);
					}
					table_class_block[i].table_class_block.push_back(table_descriptors_empty_pages[0]);
					hed_empty = (unsigned char *)table_class_block[i].table_class_block[0] - size_page + sizeof(page_header);
				}
				else{//страница полностью занята 1 блоком
					descriptor_writing((table_descriptors_empty_pages[0]), 3, 0, (amount_empty_block - 1), ((log(requested_size)) / (log(2))));
					hed_empty = (unsigned char *)table_descriptors_empty_pages[0] - size_page + sizeof(page_header);
				}
				table_descriptors_empty_pages.erase(table_descriptors_empty_pages.begin());
				
				fill_memory(hed_empty, requested_size, '*');
				return (hed_empty);
			}
			else { return NULL; }
		}
	}
	}




void mem_dump(){
	for (int l = 0; l < amount_pages; l++){
		cout << " Page # " << l<<endl;
		for (int k = 0; k < size_page; k++){
			if (pool[k + size_page*l] != '*'){
				cout << k << " " << (int)pool[k + size_page*l] << endl;
			}
			else{
				cout << k << " " <<pool[k + size_page*l] << endl;
			}
		}
		cout << " =============================================================================== "<<endl;
	}
}

void mem_free(void *addr)
{
	cout << "Free memory " << endl;
	int block=search_page_of_block(addr);
	int i = block;
	int empty_block;
	int size_class;
	int block_place_in_array;
	int offset_block_in_page;
	bool this_block_empty_flag=false;
	if (table_descriptors_pages[block]->state == 2){//страница занята многостраничным блоком
		while ((i<table_descriptors_pages.size()) && (table_descriptors_pages[i]->state == 2)){
			descriptor_writing(table_descriptors_pages[i], 0, 0, 1, 0);
			table_descriptors_empty_pages.insert(table_descriptors_empty_pages.begin()+ search_position_in_empty(table_descriptors_pages[i]), table_descriptors_pages[i]);//вставляем освободившиеся страницы в список пустых
			fill_memory((unsigned char*)table_descriptors_pages[i] - size_page + sizeof(page_header), size_page - sizeof(page_header), 0);
			i++;
		}
	}
	else if (table_descriptors_pages[block]->state==1){//страница разделена на блоки одного класса

		
		block_place_in_array = search_place_in_array_for_block(addr);
		offset_block_in_page = block_place_in_array - block*size_page;
		unsigned char m = 0;
		unsigned char* start;
		unsigned char* p_start;

		if (table_descriptors_pages[block]->amount_empty_block == 0){//полностью заполненая страница разделеная на блоки одного класса
			table_descriptors_pages[block]->pointer_1st_empty_block = offset_block_in_page;
			fill_memory((unsigned char*)table_descriptors_pages[block] - size_page + sizeof(page_header) + offset_block_in_page, pow(2,table_descriptors_pages[block]->size_class), 0);
			table_descriptors_pages[block]->amount_empty_block = 1;
			table_descriptors_pages[block]->state = 1;
			table_descriptors_pages[block]->size_class;
			//delete_from_class_table(table_descriptors_pages[block]);
			table_class_block[search_place(pow(2, table_descriptors_pages[block]->size_class))].table_class_block.push_back(table_descriptors_pages[block]);
		}
		else {//это не полностью заполненая блоками одного класса страница
			size_class = search_class_by_header(table_descriptors_pages[block]);
			if (offset_block_in_page > table_descriptors_pages[block]->pointer_1st_empty_block){//перед блоком есть пустой блок
				start = (unsigned char*)table_descriptors_pages[block] - size_page + sizeof(page_header) + table_descriptors_pages[block]->pointer_1st_empty_block;
				p_start = start;

					while (p_start + *(unsigned short*)start != &pool[block_place_in_array]){
						if (*(unsigned short*)start != 0){
							start = start + *(unsigned short*)start;
						}
						else{
							this_block_empty_flag = true;
							break;
						}
					}
					*start = offset_block_in_page;
			}else{//это будет первый свободный блок 
				pool[block_place_in_array] = table_descriptors_pages[block]->pointer_1st_empty_block;
				table_descriptors_pages[block]->pointer_1st_empty_block = offset_block_in_page;
				this_block_empty_flag = true;
			}
			table_descriptors_pages[block]->amount_empty_block++;
			if (table_descriptors_pages[block]->amount_empty_block == ((size_page - sizeof(page_header)) / size_class)){//если страница освободилась
				fill_memory((unsigned char*)table_descriptors_pages[block] - size_page + sizeof(page_header), size_page - sizeof(page_header), 0);
				table_descriptors_pages[block]->amount_empty_block=1;
				table_descriptors_pages[block]->state = 0;
				table_descriptors_pages[block]->pointer_1st_empty_block = 0;
				table_descriptors_pages[block]->size_class = 0;
				delete_from_class_table(table_descriptors_pages[block]);
				table_descriptors_empty_pages.insert(table_descriptors_empty_pages.begin() + search_position_in_empty(table_descriptors_pages[block]), table_descriptors_pages[block]);
			}
			if (this_block_empty_flag != true){//выделенный блок последний
				fill_memory((unsigned char*)table_descriptors_pages[block] + offset_block_in_page - size_page + sizeof(page_header), size_class, 0);
			}
			else{
				fill_memory((unsigned char*)table_descriptors_pages[block] + offset_block_in_page - size_page + sizeof(page_header)+1, size_class-1, 0);
			}
		}
	}
	else{//страница занята одним блоком(т.е. код 3)
		fill_memory((unsigned char*)table_descriptors_pages[block] - size_page + sizeof(page_header), size_page - sizeof(page_header), 0);
		table_descriptors_pages[block]->amount_empty_block = 1;
		table_descriptors_pages[block]->state = 0;
		table_descriptors_pages[block]->pointer_1st_empty_block = 0;
		table_descriptors_pages[block]->size_class = 0;
		table_descriptors_empty_pages.insert(table_descriptors_empty_pages.begin() + search_position_in_empty(table_descriptors_pages[block]), table_descriptors_pages[block]);	
	}
	
}



void *mem_realloc(void *addr, size_t size)
{
	unsigned int requested_size = size_align(size);
	cout << "We are trying to realloc memory " << endl;
	int block = search_page_of_block(addr);
	int i = block;
	int size_class;
	int block_place_in_array = search_place_in_array_for_block(addr);
	unsigned char copy_pool[amount_pages * (size_page-sizeof(page_header))];//создаем массив, куда будем копировать данный с максимально возможным размером
	int size_copy_pool=0;
	
	if (table_descriptors_pages[block]->state == 2){//страница занята многостраничным блоком
		while ((i<table_descriptors_pages.size()) && (table_descriptors_pages[i]->state == 2)){
			for (int t = 0; t < (size_page - sizeof(page_header)); t++){//копируем данные по страницам
				copy_pool[i*(size_page - sizeof(page_header))+t] = pool[block_place_in_array + t + i*size_page];
				size_copy_pool++;
			}
			i++;
		}
	}
	else if (table_descriptors_pages[block]->state == 1){//страница разделена на блоки одного класса
		size_class = search_class_by_header(table_descriptors_pages[block]);
		for (int t = 0; t < size_class; t++){//копируем данные по страницам
			copy_pool[t] = pool[block_place_in_array + t];
			size_copy_pool++;
		}
	}
	else{//страница занята одним блоком(т.е. код 3)
		for (int t = 0; t < size_page; t++){//копируем данные по страницам
			copy_pool[t] = pool[block_place_in_array + t];
			size_copy_pool++;
		}
	}
	mem_free(addr);
	unsigned char* p;
	p = (unsigned char*)mem_alloc(requested_size);
	if (p == NULL){//если аллок не смог выделить блоку новое кол-во памяти
		p = (unsigned char*)mem_alloc(size_class);
		for (int t = 0; t < size_class; t++){
			*(p + (unsigned char)t) = copy_pool[t];
		}
		return p;
	}
	else{//если смог выделить новую память
		for (int t = 0; t < requested_size; t++){
			*(p + (unsigned char)t) = copy_pool[t];
		}
		return NULL;
	}
}

void _init(){
	freopen("log.txt", "w", stdout);
	for (int k = 0; k < _pool_size; k++){
		pool[k] = 0;
	}
	for (int i = 0; i < amount_pages; i++)
	{
		page_header *h = (page_header *)(pool + i*size_page + (size_page - (sizeof(page_header))));
		descriptor_writing(h, 0, 0, 1, 0);


		table_descriptors_pages.push_back(h);// добавляем в список указателей

		//mem_allocg;//помещаем список указателей в память(размер списка кол-во страниц*4 байта)
		
		table_descriptors_empty_pages.push_back(h);//добавляем в список пустых страниц

		int j = 1;//добавляем для каждого класса список страниц
		pointer_block_class poin;
		do{
			poin.block_size = pow(2, j);
			table_class_block.push_back(poin);
			j++;
		} while (pow(2, j) != 4096);
	}

}

int main()
{
	_init();
	mem_dump();

	unsigned char* a;
	a = (unsigned char*)mem_alloc(3);
	mem_dump();


	unsigned char* b;
	b = (unsigned char*)mem_alloc(3);
	mem_dump();

	unsigned char* c;
	c = (unsigned char*)mem_alloc(4);
	mem_dump();

	unsigned char* d;
	d = (unsigned char*)mem_alloc(4);
	mem_dump();

	unsigned char* e;
	e = (unsigned char*)mem_alloc(16);
	mem_dump();

	mem_free(c);
	mem_dump();

	mem_free(a);
	mem_dump();

	mem_free(e);
	mem_dump();

	unsigned char* f;
	f = (unsigned char*)mem_alloc(3);
	mem_dump();

	unsigned char* k;
	k = (unsigned char*)mem_alloc(3);
	mem_dump();

	unsigned char* j;
	j = (unsigned char*)mem_alloc(3);
	mem_dump();


	/*unsigned char* a;
	a = (unsigned char*)mem_alloc(2);
	mem_dump();


	unsigned char* b;
	b = (unsigned char*)mem_alloc(3);
	mem_dump();


	unsigned char* c;
	c = (unsigned char*)mem_alloc(4);
	mem_dump();

	unsigned char* d;
	d = (unsigned char*)mem_realloc(a,3);
	mem_dump();*/


	_getch();
	return 0;
}