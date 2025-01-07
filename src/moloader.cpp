#include "moloader/moloader.hpp"

#include <bit>
#include <bits/ranges_algo.h>
#include <cstdint>
#include <cstring>
#include <fstream>
namespace moloader {
static std::vector<uint32_t> mHashTable;
static std::vector<std::string> mOriginalTable;
static std::vector<std::string> mTranslatedTable;
static bool mIsBigEndian = false;

uint32_t hashString(const char* str) {
  uint32_t hash = 0;

  for (auto p = reinterpret_cast<const uint8_t*>(str); *p; p++) {
    hash <<= 4;
    hash += *p;
    uint32_t tmp = hash & 0xF0000000;
    if (tmp != 0) {
      hash ^= tmp;
      hash ^= tmp >> 24;
    }
  }
  return hash;
}

bool readU32Be(std::ifstream& in, uint32_t& val) {
  in.read(reinterpret_cast<char*>(&val), sizeof(uint32_t));
#if __BYTE_ORDER == __BIG_ENDIAN
  if (!mIsBigEndian) {
#elif __BYTE_ORDER == __LITTLE_ENDIAN
  if (mIsBigEndian) {
#endif
    val = std::byteswap(val);
  }
  return in.good();
}

bool readU32Le(std::ifstream& in, uint32_t& val) {
  in.read(reinterpret_cast<char*>(&val), sizeof(uint32_t));
#if __BYTE_ORDER == __BIG_ENDIAN
  if (!mIsBigEndian) {
#elif __BYTE_ORDER == __LITTLE_ENDIAN
  if (mIsBigEndian) {
#endif
    val = std::byteswap(val);
  }
  return in.good();
}

bool readU32(std::ifstream& in, uint32_t& val) { return mIsBigEndian ? readU32Be(in, val) : readU32Le(in, val); }
bool load(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return false;
  }

  uint32_t magic = 0;
  in.read(reinterpret_cast<char*>(&magic), 4);
  if (magic == 0x950412de) {
    mIsBigEndian = false;
  } else if (magic == 0xde120495) {
    mIsBigEndian = true;
  } else {
    return false;
  }
  uint32_t revision;
  if (!readU32(in, revision) || revision != 0) {
    return false;
  }
  uint32_t numStrings;
  if (!readU32(in, numStrings) || numStrings == 0) {
    return false;
  }
  uint32_t origTableOffset;
  if (!readU32(in, origTableOffset)) {
    return false;
  }

  uint32_t transTableOffset;
  if (!readU32(in, transTableOffset)) {
    return false;
  }
  uint32_t hashTableSize;
  if (!readU32(in, hashTableSize)) {
    return false;
  }
  uint32_t hashTableOffset;
  if (!readU32(in, hashTableOffset)) {
    return false;
  }
  mOriginalTable.clear();
  mOriginalTable.reserve(numStrings);
  in.seekg(origTableOffset, std::ios::beg);
  for (int i = 0; i < numStrings; i++) {
    uint32_t stringLength;
    if (!readU32(in, stringLength)) {
      return false;
    }
    uint32_t stringOffset;
    if (!readU32(in, stringOffset)) {
      return false;
    }
    mOriginalTable.emplace_back();
    mOriginalTable.back().resize(stringLength);
    auto pos = in.tellg();
    in.seekg(stringOffset, std::ios::beg);
    in.read(reinterpret_cast<char*>(mOriginalTable.back().data()), stringLength + 1);
    if (in.bad()) {
      return false;
    }
    // std::cout << mOriginalTable.back() << std::endl;
    in.seekg(pos, std::ios::beg);
  }

  mTranslatedTable.clear();
  mTranslatedTable.reserve(numStrings);
  in.seekg(transTableOffset, std::ios::beg);
  for (int i = 0; i < numStrings; i++) {
    uint32_t stringLength;
    if (!readU32(in, stringLength)) {
      return false;
    }
    uint32_t stringOffset;
    if (!readU32(in, stringOffset)) {
      return false;
    }
    mTranslatedTable.emplace_back();
    mTranslatedTable.back().resize(stringLength);
    auto pos = in.tellg();
    in.seekg(stringOffset, std::ios::beg);
    in.read(reinterpret_cast<char*>(mTranslatedTable.back().data()), stringLength + 1);
    if (in.bad()) {
      return false;
    }
    // std::cout << mTranslatedTable.back() << std::endl;
    in.seekg(pos, std::ios::beg);
  }

  in.seekg(hashTableOffset, std::ios::beg);
  mHashTable.clear();
  mHashTable.resize(hashTableSize);
  in.read(reinterpret_cast<char*>(mHashTable.data()), hashTableSize * sizeof(uint32_t));
  if (in.bad()) {
    return false;
  }
#if __BYTE_ORDER == __LITTLE_ENDIAN
  if (mIsBigEndian) {
#elif __BYTE_ORDER == __BIG_ENDIAN
  if (!mIsBigEndian) {
#endif
    std::ranges::for_each(mHashTable.begin(), mHashTable.end(), [](uint32_t& val) { val = std::byteswap(val); });
  }
  return true;
}

static uint32_t calculateNextIndex(uint32_t curidx, uint32_t hsize, uint32_t step) { return curidx + step - (curidx >= hsize - step ? hsize : 0); }

const char* gettext(const char* str) {
  if (mHashTable.size() <= 2) {
    return str;
  }
    
  const uint32_t hash = hashString(str);
  const uint32_t step = (hash % (mHashTable.size() - 2)) + 1;
  uint32_t idx = hash % mHashTable.size();
  const uint32_t len = strlen(str);

  while (true) {
    uint32_t strno = mHashTable[idx];
    if (strno == 0) {
      return nullptr;
    }

    if ((strno & (1 << 31)) == 0) {
      // not a system dependent string
      if (len <= mOriginalTable[strno].size() && !strcmp(mOriginalTable[strno].data(), str)) {
        return mTranslatedTable[strno].c_str();
      }
    }
    idx = calculateNextIndex(idx, mHashTable.size(), step);
  }
}

std::string getstring(const std::string& str) {
    auto ret = gettext(str.c_str());
    return ret ? std::string(ret) : str;
}
} // namespace moloader