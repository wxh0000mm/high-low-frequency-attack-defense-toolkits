#include "TLVPackage.h"

TLVPackage::TLVPackage(bool isBuffer)
{
    bufferLength = 0;
    tagSize = 0;
    lengthSize = 0;
    valueSize = 0;

    BufferOnly = isBuffer;
}

// 原使用方法与TLV构造实体函数的接口层
void TLVPackage::Connector(
    const BYTE* pbRecv, 
    DWORD dwRecv, 
    std::list<TLVPackage *>& TLVList)
{
    TLVPackage* CurrentPackage = new TLVPackage(true);
    TLVList.push_back(CurrentPackage);

    for(unsigned int i = 0; i < dwRecv; i++) {
        CurrentPackage->buffer.push_back(pbRecv[i]);
    }
    CurrentPackage->bufferLength = dwRecv;

    TLVPackage::Construct(CurrentPackage, TLVList);
}

// 构造TLV
void TLVPackage::Construct(
    TLVPackage* WholePackage,
    std::list<TLVPackage *>& TLVList)
{
    unsigned int currentIndex = 0;
    char currentStatus = 'T';
    unsigned int valueSize = 0;
    TLVPackage *CurrentPackage = NULL;

    while(currentIndex < WholePackage->bufferLength)
    {
        switch (currentStatus) {
        case 'T':
            valueSize = 0;
            // 应对单个结构的TLV
            CurrentPackage = new TLVPackage(false);
            TLVList.push_back(CurrentPackage);

            // 判断是否单一结构
            if((WholePackage->buffer[currentIndex] & 0x20) != 0x20) {
                // 判断是否多字节Tag
                if((WholePackage->buffer[currentIndex] & 0x1f) == 0x1f) {
                    int endTagIndex = currentIndex;
                    // 判断第二个字节的最高位是否为1
                    while ((WholePackage->buffer[++endTagIndex] & 0x80) == 0x80);
                    unsigned int tagSize = endTagIndex - currentIndex + 1;

                    CurrentPackage->tagSize = tagSize;
                    for(unsigned int i = 0; i < tagSize; i++) {
                        CurrentPackage->tag.push_back(WholePackage->buffer[currentIndex + i]);
                    }
                    currentIndex += tagSize;
                } else {
                    CurrentPackage->tagSize = 1;
                    CurrentPackage->tag.push_back(WholePackage->buffer[currentIndex]);
                    currentIndex += 1;
                }
            } else {
                // 判断是否多字节Tag
                if((WholePackage->buffer[currentIndex] & 0x1f) == 0x1f) {
                    int endTagIndex = currentIndex;
                    // 判断第二个字节的最高位是否为1
                    while ((WholePackage->buffer[++endTagIndex] & 0x80) == 0x80);
                    unsigned int tagSize = endTagIndex - currentIndex + 1;

                    CurrentPackage->tagSize = tagSize;
                    for(unsigned int i = 0; i < tagSize; i++) {
                        CurrentPackage->tag.push_back(WholePackage->buffer[currentIndex + i]);
                    }
                    currentIndex += tagSize;
                } else {
                    CurrentPackage->tagSize = 1;
                    CurrentPackage->tag.push_back(WholePackage->buffer[currentIndex]);
                    currentIndex += 1;
                }
                // 分析SubTag
                unsigned int subLength = 0;
                TLVPackage *subTLV = new TLVPackage(true);
                TLVList.push_back(subTLV);
                if ((WholePackage->buffer[currentIndex] & 0x80) == 0x80) {
                    unsigned int lengthSize = WholePackage->buffer[currentIndex] & 0x7f;
                    for (unsigned int index = 0; index < lengthSize; index++) {
                        //计算Length域的长度
                        subLength += WholePackage->buffer[currentIndex + 1 + index] << (index * 8); 
                    }
                    for (unsigned int i = 0; i < subLength; i++) {
                        subTLV->buffer.push_back(WholePackage->buffer[currentIndex + i + lengthSize + 1]);
                    }
                } else {
                    subLength = WholePackage->buffer[currentIndex];
                    for (unsigned int i = 0; i < subLength; i++) {
                        subTLV->buffer.push_back(WholePackage->buffer[currentIndex + i + 1]);
                    }
                }

                subTLV->bufferLength = subLength;

                Construct(subTLV, TLVList);
            }
            currentStatus = 'L';
            break;
        case 'L':
            // 判断长度字节的最高位是否为1，
            // 如果为1，则该字节为长度扩展字节，由下一个字节开始决定长度
            if ((WholePackage->buffer[currentIndex] & 0x80) != 0x80) {
                CurrentPackage->lengthSize = 1;
                CurrentPackage->length.push_back(WholePackage->buffer[currentIndex]);
                valueSize = WholePackage->buffer[currentIndex];
                currentIndex += 1;
            } else {
                unsigned int lengthSize = WholePackage->buffer[currentIndex] & 0x7f;
                // 从下一个字节开始算Length域
                currentIndex += 1; 
                for (unsigned int index = 0; index < lengthSize; index++) {
                    // 计算Length域的长度
                    valueSize += WholePackage->buffer[currentIndex + index] << (index * 8); 
                }

                for (unsigned int i = 0; i < lengthSize; i++) {
                    CurrentPackage->length.push_back(WholePackage->buffer[currentIndex + i]);
                }
                CurrentPackage->lengthSize = lengthSize;
                currentIndex += lengthSize;
            }
            currentStatus = 'V';
            break;
        case 'V':
            for (unsigned int i = 0; i < valueSize; i++) {
                CurrentPackage->value.push_back(WholePackage->buffer[currentIndex + i]);
            }
            CurrentPackage->valueSize = valueSize;
            currentIndex += valueSize;

            //PrintTLVInfo(CurrentPackage);

            currentStatus = 'T';
            break;
        default:
            return;
        }
    }
}

void TLVPackage::PrintTLVInfo(
    const TLVPackage* CurrentPackage)
{
    if (CurrentPackage->tagSize == 2 ) {
        if ((CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x20) ||
            (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x61) ||
            (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x62) ||
            (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x2D) ||
            (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x25) ||
            (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x24) ||
            (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x12) ||
            (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x0B)) {
            goto __print;
        }
        else {
            return; 
        }
    }
    else if (CurrentPackage->tagSize == 1) {
        if (CurrentPackage->tag[0] == 0x50 ||
            CurrentPackage->tag[0] == 0x5A) {
            goto __print;
        }
        else
            return;
    }
    else
        return;
     

__print:

#ifdef TEST
    //std::cout.setf(std::ios::hex);
    std::cout << "Tag: ";
    for (int i = 0; i < CurrentPackage->TagSize; i++)
        std::wcout << std::hex << std::uppercase << CurrentPackage->Tag[i] << std::dec;
    std::wcout << std::endl;

    //std::cout.setf(std::ios::dec);
    std::wcout << "    Tag's size: " << CurrentPackage->TagSize << std::endl;

    //std::cout.setf(std::ios::hex);
    std::wcout << "    Value Hex: ";
    for (int i = 0; i < *(CurrentPackage->Length); i++)
        std::wcout << std::hex << std::uppercase << CurrentPackage->Value[i] << std::dec;
    std::wcout << std::endl;

    std::wcout << "    Value: ";
    std::cout << CurrentPackage->Value << std::endl;

    //std::cout.setf(std::ios::dec);
    unsigned char length = *(CurrentPackage->Length);
    std::wcout << "    Value's length: " << length << std::endl;
#endif

    if (CurrentPackage->tagSize == 2) {
            // 持卡人姓名
            if (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x20) {
                static bool guy_name = false;
                if (guy_name)
                    return;
                std::cout << "持卡人姓名： " << std::string(CurrentPackage->value.begin(), CurrentPackage->value.end()) << std::endl;
                guy_name = true;
            }
            else if (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x0B) {
                static bool guy_name_ex = false;
                if (guy_name_ex)
                    return;
                std::cout << "持卡人姓名扩展： " << std::string(CurrentPackage->value.begin(), CurrentPackage->value.end()) << std::endl;
                guy_name_ex = true;
            }
            // 持卡人证件号
            else if (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x61) {
                static bool id_num = false;
                if (id_num)
                    return;
                std::cout << "持卡人证件号： " << std::string(CurrentPackage->value.begin(), CurrentPackage->value.end()) << std::endl;
                id_num = true;
            }
            // 持卡人证件类型
            else if (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x62) {
                static bool id_type = false;
                if (id_type)
                    return;
                std::cout << "持卡人证件类型： ";
                if (CurrentPackage->value[0] == 0x00) {
                    std::cout << "身份证" << std::endl;
                }
                else {
                    std::cout << "未知" << std::endl;
                }
                id_type = true;
            }
            // 卡名
            else if (CurrentPackage->tag[0] == 0x9F && CurrentPackage->tag[1] == 0x12) {
                static bool card_name = false;
                if (card_name)
                    return;
                std::cout << "发卡名： " << std::string(CurrentPackage->value.begin(), CurrentPackage->value.end()) << std::endl;
                card_name = true;
            }
            // 发卡时间
            else if (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x25) {
                static bool begin_time = false;
                if (begin_time)
                    return;
                std::cout << "发卡时间： ";
                std::cout << "20";
                printf("%02X", CurrentPackage->value[0]);
                std::cout << "年";
                printf("%02X", CurrentPackage->value[1]);
                std::cout << "月";
                printf("%02X", CurrentPackage->value[2]);
                std::cout << "日" << std::endl;
                begin_time = true;
            }
            // 卡过期时间
            else if (CurrentPackage->tag[0] == 0x5F && CurrentPackage->tag[1] == 0x24) {
                static bool end_time = false;
                if (end_time)
                    return;
                std::cout << "过期时间： ";
                std::cout << "20";
                printf("%02X", CurrentPackage->value[0]);
                std::cout << "年";
                printf("%02X", CurrentPackage->value[1]);
                std::cout << "月";
                printf("%02X", CurrentPackage->value[2]);
                std::cout << "日" << std::endl;
                end_time = true;
            }
            else {
                return;
            }
        }
        else if (CurrentPackage->tagSize == 1) {
            // 持卡类型
            if (CurrentPackage->tag[0] == 0x50) {
                static bool card_type = false;
                if (card_type)
                    return;
                std::cout << "持卡类型： " << std::string(CurrentPackage->value.begin(), CurrentPackage->value.end()) << std::endl;
                card_type = true;
            }
            // 卡号
            else if (CurrentPackage->tag[0] == 0x5A) {
                static bool card_num = false;
                if (card_num)
                    return;
                std::cout << "卡号： ";
                for (unsigned int i = 0; i < CurrentPackage->valueSize; i++){
                    printf("%02X", CurrentPackage->value[i]);
                }
                printf("\n");
                card_num = true;
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
}
