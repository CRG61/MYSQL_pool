#ifndef PUBLIC_H
#define PUBLIC_H

//#define LOG(str) \
//	std::cout << __FILE__ << ":" << __LINE__ << " " << \
//	__TIMESTAMP__ << " : " << str << std::endl;

//�����б���ǻ��м�������˼�����ڶ�����е��ַ��������߿��еĺ궨��
//��б�ܺ��治�����κ��ַ����ո�Ҳ���У�
#define LOG(str) \
   std::cout << __FILE__ << ":" << __LINE__ << " " << \
    __TIMESTAMP__ << " : " << str << std::endl;

#endif