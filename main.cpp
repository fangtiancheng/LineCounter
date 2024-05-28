#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class LineCounter{
    static size_t scanFile(const fs::path& fileName){
        std::ifstream f(fileName);
        if(!f.good()){
            throw std::runtime_error("open file '"+fileName.string()+"' error");
        }
        char lastChar = '\n';
        auto testEndOfLine = [&lastChar](char c)->bool{
            lastChar = c;
            return c == '\n';
        };
        size_t lineCount = std::count_if(std::istreambuf_iterator<char>(f),
                                         std::istreambuf_iterator<char>(),
                                         testEndOfLine);
        if(lastChar != '\n') lineCount++;
        return lineCount;
    }

    static bool isTargetPostfix(const fs::path& fileName){
        return gPostfixes.find(fileName.extension().string()) != gPostfixes.cend();
    }

protected:
    inline static bool gWarninged = false;
    inline static std::set<std::string> gPostfixes{
        ".c", ".h",
        ".cpp", ".cc", ".cxx", ".hpp", ".ii", ".ixx", ".ipp", ".txx", ".tpp", ".tpl",
        ".py", ".pyw", ".pyx"
    };
    fs::path mScanDir;
    std::vector<LineCounter> mSubCounters{};
    std::map<fs::path, size_t> mFiles;
    size_t mMaxDepth;
    void privatePrint(const std::string& prefix) const {
        for(const auto& mFile: mFiles){
            std::cout << prefix << "|-- " << mFile.first << " " << mFile.second << '\n';
        }
        for(const auto& mCounter: mSubCounters){
            mCounter.privatePrint("| ");
        }
    }
public:
    LineCounter(fs::path scanDir, size_t maxDepth): mScanDir(std::move(scanDir)), mMaxDepth(maxDepth){
        for(auto& currentPath: fs::directory_iterator(mScanDir)){
            if(currentPath.is_directory()){
                if(mMaxDepth > 0) {
                    mSubCounters.emplace_back(currentPath, mMaxDepth - 1);
                } else {
                    if(!gWarninged){
                        gWarninged = true;
                        std::cerr << "[WARNING] reach max depth at " << currentPath << std::endl;
                    }
                }
            } else if(currentPath.is_regular_file()){
                if(isTargetPostfix(currentPath)) {
                    mFiles.emplace(currentPath, scanFile(currentPath));
                }
            }
        }
    };

    [[nodiscard]] size_t countLines() const {
        size_t result = 0;
        for(const auto& mFile : mFiles){
            result += mFile.second;
        }
        for(const auto& mCounter: mSubCounters){
            result += mCounter.countLines();
        }
        return result;
    }

    [[nodiscard]] size_t countFiles() const {
        size_t result = mFiles.size();
        for(const auto& mCounter: mSubCounters){
            result += mCounter.countFiles();
        }
        return result;
    }

    void print() const {
        privatePrint("");
        std::cout << "total " << countFiles() << " files, " << countLines() << " lines" << std::endl;
    }
};

int main(int argc, char* argv[]){
    fs::path targetDir = "./";
    if(argc == 2){
        targetDir = argv[1];
    } else if(argc > 2){
        std::cerr << "usage: " << argv[0] << " ${target_scan_dir}\n";
        return 1;
    }
    LineCounter counter(targetDir, 20);
    counter.print();
    return 0;
}