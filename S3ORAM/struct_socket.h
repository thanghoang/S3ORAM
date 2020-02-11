/*
 * struct_socket.h
 *
 *  Created on: Apr 27, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
#ifndef STRUCT_SOCKET_H
#define STRUCT_SOCKET_H

struct struct_socket
{
	std::string ADDR;
	
	unsigned char* data_out;
	size_t data_out_size;
	bool isSend;
    unsigned char* data_in;
	size_t data_in_size;
    int peer_idx;
    
	int CMD;
	
    
    struct_socket(int peer_idx, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD, bool isSend)
	{
        this->peer_idx = peer_idx;
		
		this->data_out = data_out;
		this->data_out_size = data_out_size;
		
		this->CMD = CMD;
        this->data_in = data_in;
        this->data_in_size = data_in_size;
        
        this->isSend = isSend;
	}
	struct_socket()
	{
	}
	~struct_socket()
	{
	}

};

#endif // STRUCT_SOCKET_H
