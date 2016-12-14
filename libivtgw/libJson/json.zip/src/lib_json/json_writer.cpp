#include <json/writer.h>
#include <utility>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#if _MSC_VER >= 1400 // VC++ 8.0
#pragma warning( disable : 4996 )   // disable warning about strdup being deprecated.
#endif

namespace Json {

static void uintToString( unsigned int value, 
                          char *&current )
{
   *--current = 0;
   do
   {
      *--current = (value % 10) + '0';
      value /= 10;
   }
   while ( value != 0 );
}

void valueToString(std::string& document, Value::Int value )
{
   char buffer[32];
   char *current = buffer + sizeof(buffer);
   bool isNegative = value < 0;
   if ( isNegative )
      value = -value;
   uintToString( Value::UInt(value), current );
   if ( isNegative )
      *--current = '-';
   assert( current >= buffer );

	document += current;
}


void valueToString(std::string& document, Value::UInt value )
{
   char buffer[32];
   char *current = buffer + sizeof(buffer);
   uintToString( value, current );
   assert( current >= buffer );

	document += current;
}

void valueToString(std::string& document, double value )
{
   char buffer[32];
#ifdef __STDC_SECURE_LIB__ // Use secure version with visual studio 2005 to avoid warning.
   sprintf_s(buffer, sizeof(buffer), "%.16g", value); 
#else	
   sprintf(buffer, "%.16g", value); 
#endif

	document += buffer;
}


void valueToString(std::string& document, bool value )
{
	document += (value ? "true" : "false");
}

void valueToQuotedString(std::string& document, const char *value )
{
	document += "\"";
	const char * cur = value;
	while(*cur)
	{
		switch(*cur)
		{
	        case '\"': document += "\\\""; break;
			case '\\': document += "\\\\"; break;
			case '/': document += "\\/"; break;
			case '\b': document += "\\b"; break;
			case '\f': document += "\\f"; break;
			case '\n': document += "\\n"; break;
			case '\r': document += "\\r"; break;
			case '\t': document += "\\t"; break;
			default: document += *cur; break;
		}
		cur++;
	}
	document += "\"";
}


// Class FastWriter
// //////////////////////////////////////////////////////////////////

FastWriter::FastWriter(std::string& output)
: document_(output)
{
}


bool 
FastWriter::write( const Value &root )
{
   document_ = "";
   writeValue( root );
   document_ += "\n";
   return true;
}


void 
FastWriter::writeValue( const Value &value )
{
	switch ( value.type() )
	{
	case nullValue:
		document_ += "null";
		break;
	case intValue:
		valueToString( document_, value.asInt() );
		break;
	case uintValue:
		valueToString( document_, value.asUInt() );
		break;
	case realValue:
		valueToString( document_, value.asDouble() );
		break;
	case stringValue:
		valueToQuotedString( document_, value.asCString() );
		break;
	case booleanValue:
		valueToString( document_, value.asBool() );
		break;
	case arrayValue:
		{
			document_ += "[ ";
			int size = value.size();
			for ( int index =0; index < size; ++index )
			{
				if ( index > 0 )
					document_ += ", ";
				writeValue( value[index] );
			}
			document_ += " ]";
		}
		break;
	case objectValue:
		{
			Value::Members members( value.getMemberNames() );
			document_ += "{ ";
			for ( Value::Members::iterator it = members.begin(); 
				it != members.end(); 
				++it )
			{
				const std::string &name = *it;
				if ( it != members.begin() )
					document_ += ", ";
				valueToQuotedString( document_, name.c_str() );
				document_ += " : ";
				writeValue( value[name] );
			}
			document_ += " }";
		}
		break;
	}
}


// Class StyledWriter
// //////////////////////////////////////////////////////////////////

StyledWriter::StyledWriter(std::string& output)
   : document_(output)
   , rightMargin_( 74 )
   , indentSize_( 3 )
{
}


bool 
StyledWriter::write( const Value &root )
{
   document_ = "";
   addChildValues_ = false;
   indentString_ = "";
   writeCommentBeforeValue( root );
   writeValue( root );
   writeCommentAfterValueOnSameLine( root );
   document_ += "\n";
   return true;
}


void 
StyledWriter::writeValue( const Value &value )
{
	std::string strVal = "";

	switch ( value.type() )
	{
	case nullValue:
		pushValue( "null" );
		break;
	case intValue:
		valueToString( strVal, value.asInt() );
		pushValue( strVal );
		break;
	case uintValue:
		valueToString( strVal, value.asUInt() );
		pushValue( strVal );
		break;
	case realValue:
		valueToString( strVal, value.asDouble() );
		pushValue( strVal );
		break;
	case stringValue:
		valueToQuotedString( strVal, value.asCString() );
		pushValue( strVal );
		break;
	case booleanValue:
		valueToString( strVal, value.asBool() );
		pushValue( strVal );
		break;
	case arrayValue:
		writeArrayValue( value);
		break;
	case objectValue:
		{
			Value::Members members( value.getMemberNames() );
			if ( members.empty() )
				pushValue( "{}" );
			else
			{
				writeWithIndent( "{" );
				indent();
				Value::Members::iterator it = members.begin();
				while ( true )
				{
					const std::string &name = *it;
					const Value &childValue = value[name];
					writeCommentBeforeValue( childValue );
					strVal = "";
					valueToQuotedString( strVal, name.c_str() );
					writeWithIndent( strVal );
					document_ += " : ";
					writeValue( childValue );
					if ( ++it == members.end() )
					{
						writeCommentAfterValueOnSameLine( childValue );
						break;
					}
					document_ += ",";
					writeCommentAfterValueOnSameLine( childValue );
				}
				unindent();
				writeWithIndent( "}" );
			}
		}
		break;
	}
}


void 
StyledWriter::writeArrayValue( const Value &value )
{
   int size = value.size();
   if ( size == 0 )
      pushValue( "[]" );
   else
   {
      bool isArrayMultiLine = isMultineArray( value );
      if ( isArrayMultiLine )
      {
         writeWithIndent( "[" );
         indent();
         bool hasChildValue = !childValues_.empty();
         int index =0;
         while ( true )
         {
            const Value &childValue = value[index];
            writeCommentBeforeValue( childValue );
            if ( hasChildValue )
               writeWithIndent( childValues_[index] );
            else
            {
               writeIndent();
               writeValue( childValue );
            }
            if ( ++index == size )
            {
               writeCommentAfterValueOnSameLine( childValue );
               break;
            }
            document_ += ",";
            writeCommentAfterValueOnSameLine( childValue );
         }
         unindent();
         writeWithIndent( "]" );
      }
      else // output on a single line
      {
         assert( childValues_.size() == size );
         document_ += "[ ";
         for ( int index =0; index < size; ++index )
         {
            if ( index > 0 )
               document_ += ", ";
            document_ += childValues_[index];
         }
         document_ += " ]";
      }
   }
}


bool 
StyledWriter::isMultineArray( const Value &value )
{
   int size = value.size();
   bool isMultiLine = size*3 >= rightMargin_ ;
   childValues_.clear();
   for ( int index =0; index < size  &&  !isMultiLine; ++index )
   {
      const Value &childValue = value[index];
      isMultiLine = isMultiLine  ||
                     ( (childValue.isArray()  ||  childValue.isObject())  &&  
                        childValue.size() > 0 );
   }
   if ( !isMultiLine ) // check if line length > max line length
   {
      childValues_.reserve( size );
      addChildValues_ = true;
      int lineLength = 4 + (size-1)*2; // '[ ' + ', '*n + ' ]'
      for ( int index =0; index < size  &&  !isMultiLine; ++index )
      {
         writeValue( value[index] );
         lineLength += int( childValues_[index].length() );
         isMultiLine = isMultiLine  &&  hasCommentForValue( value[index] );
      }
      addChildValues_ = false;
      isMultiLine = isMultiLine  ||  lineLength >= rightMargin_;
   }
   return isMultiLine;
}


void 
StyledWriter::pushValue( const std::string &value )
{
   if ( addChildValues_ )
      childValues_.push_back( value );
   else
      document_ += value;
}


void 
StyledWriter::writeIndent()
{
   if ( !document_.empty() )
   {
      char last = document_[document_.length()-1];
      if ( last == ' ' )     // already indented
         return;
      if ( last != '\n' )    // Comments may add new-line
         document_ += '\n';
   }
   document_ += indentString_;
}


void 
StyledWriter::writeWithIndent( const std::string &value )
{
   writeIndent();
   document_ += value;
}


void 
StyledWriter::indent()
{
   indentString_ += std::string( indentSize_, ' ' );
}


void 
StyledWriter::unindent()
{
   assert( int(indentString_.size()) >= indentSize_ );
   indentString_.resize( indentString_.size() - indentSize_ );
}


void 
StyledWriter::writeCommentBeforeValue( const Value &root )
{
   if ( !root.hasComment( commentBefore ) )
      return;
   document_ += normalizeEOL( root.getComment( commentBefore ) );
   document_ += "\n";
}


void 
StyledWriter::writeCommentAfterValueOnSameLine( const Value &root )
{
   if ( root.hasComment( commentAfterOnSameLine ) )
      document_ += " " + normalizeEOL( root.getComment( commentAfterOnSameLine ) );

   if ( root.hasComment( commentAfter ) )
   {
      document_ += "\n";
      document_ += normalizeEOL( root.getComment( commentAfter ) );
      document_ += "\n";
   }
}


bool 
StyledWriter::hasCommentForValue( const Value &value )
{
   return value.hasComment( commentBefore )
          ||  value.hasComment( commentAfterOnSameLine )
          ||  value.hasComment( commentAfter );
}


std::string 
StyledWriter::normalizeEOL( const std::string &text )
{
   std::string normalized;
   normalized.reserve( text.length() );
   const char *begin = text.c_str();
   const char *end = begin + text.length();
   const char *current = begin;
   while ( current != end )
   {
      char c = *current++;
      if ( c == '\r' ) // mac or dos EOL
      {
         if ( *current == '\n' ) // convert dos EOL
            ++current;
         normalized += '\n';
      }
      else // handle unix EOL & other char
         normalized += c;
   }
   return normalized;
}


} // namespace Json
