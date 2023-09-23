#define NDEBUG 
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>


using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}
bool PreprocessVectorInclude(const path& in_file, const path& out_file, const vector<path>& include_directories, uint32_t number_lines, string path);
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    static regex include_file(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");    // #include "..." файл
    smatch m_file;
    static regex include_library(R"/(\s*#\s*include\s*<([^>]*)>\s*)/"); // #include <...> библиотека
    uint32_t number_lines = 0;
    ifstream input_file(in_file);
    smatch m;
    if (!input_file)
    {
        return false;
    }
    ofstream output_file(out_file, ios::out | ios::app);
    string line;
    while (getline(input_file, line))
    {
        number_lines++;


        if (regex_match(line, m, include_file))
        {
            path parent_path = in_file.parent_path();
            ifstream sub_file(parent_path / string(m[1]));
            if (sub_file)
            {
                Preprocess(parent_path / string(m[1]), out_file, include_directories);

            }
            else
            {
                if (!PreprocessVectorInclude(in_file, out_file, include_directories, number_lines, string(m[1])))
                    return false;
               
            }
        }
        else if (regex_match(line, m, include_library))
        {

            if (!PreprocessVectorInclude(in_file, out_file, include_directories, number_lines, string(m[1])))
                return false;


        }
        else
        {
            output_file << line << endl;
        }
    }
    return true;
}

bool PreprocessVectorInclude(const path& in_file, const path& out_file, const vector<path>& include_directories, uint32_t number_lines, string file_name)
{

    bool found = false;
    for (auto& include : include_directories)
    {
        if (!std::filesystem::exists(path(include / file_name))) {
            continue;
        }
        found = true;
        Preprocess(include / file_name, out_file, include_directories);
        break;

    }
    if (!found)
    {
        cout << "unknown include file "s << file_name << " at file "s << in_file.string() << " at line "s << number_lines << endl;
    }
    return found;

}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
            "#include \"dir1/b.h\"\n"
            "// text between b.h and c.h\n"
            "#include \"dir1/d.h\"\n"
            "\n"
            "int SayHello() {\n"
            "    cout << \"hello, world!\" << endl;\n"
            "#   include<dummy.txt>\n"
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
            "#include \"subdir/c.h\"\n"
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
            "#include <std1.h>\n"
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
            "#include \"lib/std2.h\"\n"
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }
    ofstream file("sources"_p / "a.in"_p);
    file.close();
    Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p });
    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    ostringstream test_out;
    test_out << "// this comment before include\n"
        "// text from b.h before include\n"
        "// text from c.h before include\n"
        "// std1\n"
        "// text from c.h after include\n"
        "// text from b.h after include\n"
        "// text between b.h and c.h\n"
        "// text from d.h before include\n"
        "// std2\n"
        "// text from d.h after include\n"
        "\n"
        "int SayHello() {\n"
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}