#ifndef APPLICATION_H_
#define APPLICATION_H_


#include "window.h"


namespace pwg
{
	class Application
	{
	public:
		Application();
		~Application();

		void Run();

	private:
		pwg::Window m_window;
		void InitApplication();
		
	};
}
#endif // !APPLICATION_H_