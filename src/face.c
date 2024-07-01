

#include <Python.h>

#define WGET_CMD "wget http://192.168.1.105:8080/?action=snapshot -O /tmp/SearchFace.jpg"
#define SEARCHFACE_FILE "/tmp/SearchFace.jpg"

void face_init(void)
{
	 Py_Initialize();    
	// 初始化
	
	PyObject *sys = PyImport_ImportModule("sys");// 获取sys模块
	 PyObject *path = PyObject_GetAttrString(sys, "path");// 获取sys.path属性  
	 PyList_Append(path, PyUnicode_FromString(".")); // 将当前路径添加到sys.path中 


}

void face_final(void)
{
	Py_Finalize();// 结束Python解释器 
}

double face_category(void)//调用face.py函数
{
	double result = 0.0;
	system(WGET_CMD);//拍照
	if(0 != access(SEARCHFACE_FILE,F_OK))
	{
		return result;

	}

	 // 导入face模块
	PyObject *pModule = PyImport_ImportModule("face");
	 if (!pModule)
	 {
	 	PyErr_Print();
	 	printf("ERROR: failed to load face.py\n");
	 	
		goto FAILED_MODULE;
	 }
	 // 获取alibaba_face函数对象
	PyObject *pFunc = PyObject_GetAttrString(pModule, "alibaba_face");
	 if (!pFunc)
	 {
	 	PyErr_Print();
	 	printf("ERROR: function alibaba_face not found or not callable\n");
	 	goto FAILED_FUNC;
	 }


	 // 调用alibaba_face函数并获取返回值
	PyObject *pValue = PyObject_CallObject(pFunc, NULL);
	 if (!pValue)
	{
		PyErr_Print();
		printf("ERROR: function call failed\n");
	 	goto FAILED_VALUE;
		
		
	 }
	
	 if (!PyArg_Parse(pValue,"d",&result))//解析获取调用alibaba_face函数的返回值，转行成c语言格式
	 {
		 PyErr_Print();
		 printf("Error: parse failed");
		
		 goto FAILED_RESULT;
	 }
	
	 printf("result=%0.2lf\n",result);
	

	 
	 // 释放所有引用的Python对象
	 
FAILED_RESULT:

	Py_DECREF(pValue);

FAILED_VALUE:

	Py_DECREF(pFunc);

FAILED_FUNC:

   	Py_DECREF(pModule);

FAILED_MODULE:

   	return result;

}


