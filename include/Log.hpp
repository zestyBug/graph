/**
 * @file Log.hpp
 * @author your name (you@domain.com)
 * @brief This bring a simple stable text output interface, not only for debuging and output
 * @version 0.1
 * @date 2022-06-9
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef LOG_H
#define LOG_H 1
// #if IS_ANDROID_OS
// 	#include <android/log.h>
// 	#ifndef LOG_TAG
// 		#define LOG_TAG "hello-jni"
// 	#endif
// 	#define DEBUG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
// #else
// 	#define DEBUG(...) printf(__VA_ARGS__);
// #endif
#include "CrossPlatform.h"


#include <iostream>
#include <mutex>


namespace Tiz
{
	class Logger final
	{
	public:
		enum priority_levels:char
		{
			// print developer information (eg:which function is calling and parameters and return value)
			developer=0,
			// debug information (not product usage)
			debug,
			// detailed program information
			info,
			// an under the control problem
			notice,
			// considerable problem (eg: network problem,bad configuration), may be terminated
			warn,
			// handleable unexpected error in codes, will be terminated gradually
			error,
			// unhandleable unexpected error, will be terminated
			crit,
			// error occured from low-level (eg:OS, Hardware), terminate imediately
			emerg,
			// print the error any way
			all=63,
		};
    protected:
		static const char* desc[];
		priority_levels current_priority;
	public:
		Logger()
        {
            current_priority=developer;
        }
		void flush(){fflush(stdout);}
		template<typename T>
		Logger & operator<< (T v)
		{
			//lock.lock();
			std::cout << v;
			//lock.unlock();
			return *this;
		}
		inline Logger & operator<< (Logger &(*f)(Logger &))
            {return f(*this);}
		static inline Logger & endl(Logger &thiz)
            {return (thiz << "\n");}
		//errno
		// Logger & errnoReason(priority_levels p,int err);
		static const char* errnoReason(int err);
		//stdarg
		template<typename ... Arg>
		Logger & operator() (priority_levels p,const char* __restrict temp, Arg... args)
		{
			if(current_priority <= p)
			{
				//this->lock.lock();
				printf(temp,args...);
				//this->lock.unlock();
			}
			return *this;
		}
		//stdarg
		Logger & operator() (priority_levels p,const char* __restrict temp)
		{
			if(current_priority <= p)
			{
				//this->lock.lock();
				printf("%s",temp);
				//this->lock.unlock();
			}
			return *this;
		}
		//
		Logger & operator() (const char* __restrict temp)
		{
			//this->lock.lock();
			printf("%s",temp);
			//this->lock.unlock();
			return *this;
		}
		// JavaScript like logging
		constexpr void log (){}
		// JavaScript like logging
		template<typename T>
		void log (T v)
		{
			//this->lock.lock();
			std::cout << v;
			//this->lock.unlock();
		}
		// JavaScript like logging
		template<typename T,typename... Arg>
		inline void log (T v, const Arg... args)
			{log(v);log(args...);}
	};
}
extern Tiz::Logger Console;
#endif
