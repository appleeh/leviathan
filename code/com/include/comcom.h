/*

 ���� ifdef ����� DLL���� ���������ϴ� �۾��� ���� �� �ִ� ��ũ�θ� ����� 
 ǥ�� ����Դϴ�. �� DLL�� ��� �ִ� ������ ��� ����ٿ� ���ǵ� JCCOM_EXPORTS ��ȣ��
 �����ϵǸ�, ������ DLL�� ����ϴ� �ٸ� ������Ʈ������ �� ��ȣ�� ������ �� �����ϴ�.
 �̷��� �ϸ� �ҽ� ���Ͽ� �� ������ ��� �ִ� �ٸ� ��� ������Ʈ������ 
 JCCOM_API �Լ��� DLL���� �������� ������ ����,
 �� DLL�� �ش� ��ũ�η� ���ǵ� ��ȣ�� ���������� ������ ���ϴ�.
*/


#include "types.h"




#ifdef	WIN32

 #ifdef JCCOM_EXPORTS
  #define JCCOM_API	__declspec(dllexport)
 #else
  #define JCCOM_API	__declspec(dllimport)
 #endif
  #define DLLEXPORT	__declspec(dllexport)

#else
 #define JCCOM_API
 #define DLLEXPORT
#endif



