#ifndef STRNG_INC
#define STRNG_INC

#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#include "String.h"


class String
{
private:
	char *str;
public:
	String() { str = NULL; }

	String ( const char *val ) 
	  { if (val!=NULL) str = strdup ( val ); 
	    else str = NULL; }

	String ( const String& s ) 
	  {
	    str = NULL;
	    operator=(s);
	  }

	~String()
	  { SetNull(); }
	  
	void SetNull()
	{ if ( str != NULL ) { free ( str ); str = NULL; } }

	operator const char *() const
	{ return str; }

	char operator[] (int index)   { return str[index]; }

	const String& operator = ( const char *val )
	{ 
	  SetNull(); 
	  if ( val != NULL ) 
	  {
	    str = strdup ( val ); assert(str!=NULL);
	  } 
	  return *this; 
	}

	const String& operator = ( const String& s )
	{
	  return operator=(s.AsChars());
	}
	int isEqual ( const String& val ) const
	{ return !strcmp ( str, val ); }
	int operator == ( const String& val ) const
	{ return !strcmp ( str, val ); }

	const char *AsChars() const
	  { return str; }

	int length() const
	  { return (str!=NULL)?strlen(str):0; }

	 void operator += ( const char *val )
	  { 
	    String old;
	    old.str = str;
	    str = (char*)malloc(length()+strlen(val)+1);
	    assert(str!=NULL);
	    strcpy(str,old.str);
	    strcat(str,val);
	  }
};


#endif
