#include <json/json.h>
#include <algorithm> // sort
#include <stdio.h>

#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include "assert.h"
#endif

#if defined(_MSC_VER)  &&  _MSC_VER >= 1310
# pragma warning( disable: 4996 )     // disable fopen deprecation warning
#endif

static std::string
readInputTestFile( const char *path )
{
   FILE *file = fopen( path, "rb" );
   if ( !file )
      return std::string("");
   fseek( file, 0, SEEK_END );
   int size = ftell( file );
   fseek( file, 0, SEEK_SET );
   std::string text;
   char *buffer = new char[size+1];
   buffer[size] = 0;
   if ( fread( buffer, 1, size, file ) == size )
      text = buffer;
   fclose( file );
   delete[] buffer;
   return text;
}


static void
printValueTree( FILE *fout, Json::Value &value, const std::string &path = "." )
{
   switch ( value.type() )
   {
   case Json::nullValue:
      fprintf( fout, "%s=null\n", path.c_str() );
      break;
   case Json::intValue:
      fprintf( fout, "%s=%d\n", path.c_str(), value.asInt() );
      break;
   case Json::uintValue:
      fprintf( fout, "%s=%u\n", path.c_str(), value.asUInt() );
      break;
   case Json::realValue:
      fprintf( fout, "%s=%.16g\n", path.c_str(), value.asDouble() );
      break;
   case Json::stringValue:
      fprintf( fout, "%s=\"%s\"\n", path.c_str(), value.asString().c_str() );
      break;
   case Json::booleanValue:
      fprintf( fout, "%s=%s\n", path.c_str(), value.asBool() ? "true" : "false" );
      break;
   case Json::arrayValue:
      {
         fprintf( fout, "%s=[]\n", path.c_str() );
         int size = value.size();
         for ( int index =0; index < size; ++index )
         {
            static char buffer[16];
            sprintf( buffer, "[%d]", index );
            printValueTree( fout, value[index], path + buffer );
         }
      }
      break;
   case Json::objectValue:
      {
         fprintf( fout, "%s={}\n", path.c_str() );
         Json::Value::Members members( value.getMemberNames() );
         std::sort( members.begin(), members.end() );
         std::string suffix = *(path.end()-1) == '.' ? "" : ".";
         for ( Json::Value::Members::iterator it = members.begin(); 
               it != members.end(); 
               ++it )
         {
            const std::string &name = *it;
            printValueTree( fout, value[name], path + suffix + name );
         }
      }
      break;
   default:
      break;
   }
}


static int
parseAndSaveValueTree( const std::string &input, 
                       const std::string &actual,
                       const std::string &kind,
                       Json::Value &root )
{
   Json::Reader reader;
   bool parsingSuccessful = reader.parse( input, root );
   if ( !parsingSuccessful )
   {
      printf( "Failed to parse %s file: \n%s\n", 
              kind.c_str(),
              reader.getFormatedErrorMessages().c_str() );
      return 1;
   }

   FILE *factual = fopen( actual.c_str(), "wt" );
   if ( !factual )
   {
      printf( "Failed to create %s actual file.\n", kind.c_str() );
      return 2;
   }
   printValueTree( factual, root );
   fclose( factual );
   return 0;
}


static int
rewriteValueTree( const std::string &rewritePath, 
                  const Json::Value &root, 
                  std::string &rewrite )
{
//   Json::FastWriter writer;
   Json::StyledWriter writer(rewrite);
   writer.write( root );
   FILE *fout = fopen( rewritePath.c_str(), "wt" );
   if ( !fout )
   {
      printf( "Failed to create rewrite file: %s\n", rewritePath.c_str() );
      return 2;
   }
   fprintf( fout, "%s\n", rewrite.c_str() );
   fclose( fout );
   return 0;
}


static std::string
removeSuffix( const std::string &path, 
              const std::string &extension )
{
   if ( extension.length() >= path.length() )
      return std::string("");
   std::string suffix = path.substr( path.length() - extension.length() );
   if ( suffix != extension )
      return std::string("");
   return path.substr( 0, path.length() - extension.length() );
}

static int testEncodeDecode()
{
	Json::Value valueInput = "\"\\/\b\f\n\r\t";
	
	std::string str;
	Json::FastWriter writer(str);
	writer.write(valueInput);

	FILE* fp = fopen("a.txt", "wb");
	fwrite(str.c_str(), str.size(), 1, fp);
	fclose(fp);

	char buf[32];
	fp = fopen("a.txt", "rb");
	fread(buf, 32, 1, fp);
	fclose(fp);

	Json::Reader reader;
	Json::Value valueOutput;

	reader.parse(buf, valueOutput);

	if(valueInput == valueOutput)
	{
		printf("string encode and decode works well!\n");
	}

	return 0;
}

void Test2()
{
	Json::Value value_root;
	Json::Value value_device(Json::arrayValue);
	Json::Value device_elem;
	
	device_elem["sn"] = "sn_0";
	device_elem["devname"] = "dev_0";
	device_elem["password"] = "passwd_0";
	device_elem["time"] = 0;
	value_device.append(device_elem);

	device_elem["sn"] = "sn_1";
	device_elem["devname"] = "dev_1";
	device_elem["password"] = "passwd_1";
	device_elem["time"] = 1;
	value_device.append(device_elem);

	device_elem["sn"] = "sn_2";
	device_elem["devname"] = "dev_2";
	device_elem["password"] = "passwd_2";
	device_elem["time"] = 2;
	value_device.append(device_elem);

	Json::Value* tmp_value = &value_device[2];
	(*tmp_value)["sn"] = "sn_2_2";

	value_root["device"] = value_device;
	
	printf("device.size:%d", value_device.size());

	std::string str;
	Json::StyledWriter writer(str);
	writer.write(value_root);

	FILE* fp = fopen("device.txt", "wb");
	fwrite(str.c_str(), str.size(), 1, fp);
	fclose(fp);
	getchar();
}

void Test3( int argc, const char *argv[] )
{
	Json::Value root;
	char* filename = (char*)&argv[1][0];
	std::string output_file = (char*)&argv[2][0];
	std::string input_str = readInputTestFile(filename);
	if ( input_str == "")
	{
		printf("json setup:open file[%s] failed!\n", filename);
		return ;
	}
	
	//!分析json对象
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( input_str, root );
	if ( !parsingSuccessful )
	{
		printf( "json setup:Failed to parse %s file: \n%s\n", filename,
			reader.getFormatedErrorMessages().c_str() );
		return;
	}

	Json::Value& comand = root["Commands"];
	for (int i = 0; i < comand.size(); i++)
	{
		printf("--i:%d, value:%s\n", i, comand[i].asCString());
	}

	//!构造新的json tree
	Json::Value new_root;
	new_root["Devices"] = root["Devices"];
	Json::Value& new_command = new_root["Commands"];

	bool	bCfg = false; 

	for(int k = 3; k < argc; k++)
	{
		if ( strcmp(argv[k],  "-use") == 0 )
		{
			//!选择字符串
			char* p_use = (char*)&argv[k+1][0];
			for (int i = 0; i < comand.size(); i++)
			{
				if ( strstr(comand[i].asCString(), p_use) != 0 )
				{
					new_command.append( comand[i].asCString() );
				}
			}
			k++;
		}
		else if ( strcmp(argv[k],  "-cfg") == 0 )
		{
			bCfg = true;
			FILE* fp = fopen(output_file.c_str(), "wt+");
			if (fp == NULL )
			{
				printf("failed create file[%s]\n", output_file.c_str());
				return ;
			}
			char item[6][50];
			for (int i = 0; i < comand.size(); i++)
			{
				sscanf(comand[i].asCString(), "%s %s %s %s %s %s", item[0], item[1], item[2], item[3], item[4], item[5]);
				for (int j =0; j < 6; j++)
				{
					printf("--item[%d]:%s", j, item[j]);
				}
				printf("\n");
			}

			fclose(fp);
			break;
		}
	}

	std::string output_str;
	if( 0 != rewriteValueTree(output_file, new_root, output_str) )
	{
		printf("failed create file[%s]\n", output_file.c_str());
		return ;
	}
	printf("success create file[%s]\n", output_file.c_str());
}

int main( int argc, const char *argv[] )
{
   //testEncodeDecode();

   Test3(argc, argv);
   return 0;

   if ( argc != 2 )
   {
      printf( "Usage: %s input-json-file", argv[0] );
      return 3;
   }

   std::string input = readInputTestFile( argv[1] );
   if ( input.empty() )
   {
      printf( "Failed to read input or empty input: %s\n", argv[1] );
      return 3;
   }

   std::string basePath = removeSuffix( argv[1], ".json" );
   if ( basePath.empty() )
   {
      printf( "Bad input path. Path does not end with '.expected':\n%s\n", argv[1] );
      return 3;
   }

   std::string actualPath = basePath + ".actual";
   std::string rewritePath = basePath + ".rewrite";
   std::string rewriteActualPath = basePath + ".actual-rewrite";

   Json::Value root;
   int exitCode = parseAndSaveValueTree( input, actualPath, "input", root );
   if ( exitCode == 0 )
   {
      std::string rewrite;
      exitCode = rewriteValueTree( rewritePath, root, rewrite );
      if ( exitCode == 0 )
      {
         Json::Value rewriteRoot;
         exitCode = parseAndSaveValueTree( rewrite, rewriteActualPath, "rewrite", rewriteRoot );
      }
   }

   return exitCode;
}