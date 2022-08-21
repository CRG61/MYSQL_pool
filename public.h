#ifndef PUBLIC_H
#define PUBLIC_H

//#define LOG(str) \
//	std::cout << __FILE__ << ":" << __LINE__ << " " << \
//	__TIMESTAMP__ << " : " << str << std::endl;

//这个反斜杠是换行继续的意思，用于定义跨行的字符串，或者跨行的宏定义
//反斜杠后面不能有任何字符（空格也不行）
#define LOG(str) \
   std::cout << __FILE__ << ":" << __LINE__ << " " << \
    __TIMESTAMP__ << " : " << str << std::endl;

#endif