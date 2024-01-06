#ifndef __HNSD_STORAGE_MANAGER_H__
#define __HNSD_STORAGE_MANAGER_H__

#include <string>
#include <vector>
#include <map>

typedef enum HNSDStorageManagerResultEnum
{
    HNSDSM_RESULT_SUCCESS,
    HNSDSM_RESULT_FAILURE,
    HNSDSM_RESULT_NOT_FOUND
}HNSDSM_RESULT_T;

typedef enum HNSDStorageManagerFileType
{
    HNSDSM_FT_NOTSET,
    HNSDSM_FT_JPEG
}HNSDSM_FT_T;

class HNSDStorageFile
{
    public:
        HNSDStorageFile();
       ~HNSDStorageFile();

        void setID( std::string id );
        void setOwnerID( std::string id );
        void setInstanceID( std::string id );
        void setPurpose( std::string purpose );

        void setType( HNSDSM_FT_T type );
        void setPath( std::string path );
        void setIndex( uint index );

        void setTimestamp( uint64_t timestamp );
        void setTimestampFromSystem();

        std::string getID();
        std::string getOwnerID();
        std::string getInstanceID();                
        std::string getPurpose();

        std::string getFilename();

        uint64_t getTimestamp();
        std::string getTimestampAsUSStr();

        std::string getLocalFilePath();

        //void getInfo( std::string &fileName, std::string &purpose, std::string &tsStr );

    private:
        std::string m_id;

        std::string m_ownerID;

        std::string m_instanceID;

        std::string m_purpose;

        HNSDSM_FT_T m_type;

        std::string m_path;

        uint m_indexNum;

        uint64_t m_timestamp;

};

class HNSDStorageManager
{
    public:
        HNSDStorageManager();
       ~HNSDStorageManager();

        HNSDSM_RESULT_T start();
        HNSDSM_RESULT_T stop();

        HNSDSM_RESULT_T allocateNewFile( std::string ownerID, std::string instanceID, std::string purpose, HNSDStorageFile **filePtr );

        HNSDSM_RESULT_T findFile( std::string fileID, HNSDStorageFile **filePtr );

        virtual std::string getStorageRootPath();

        HNSDSM_RESULT_T getLocalFilePath( std::string fileID, std::string &rstPath );

        HNSDSM_RESULT_T getFileIDListForOwner( std::string ownerID, std::vector< std::string > &fileIDList );

        void deleteFile( std::string fileID );

        std::string getFileListJSON();
        std::string getFileJSON( std::string fileID );

    private:

        // The root path for storing images
        std::string m_imageStoragePath;

        uint m_nextFileIndex;

        std::map< std::string, HNSDStorageFile* > m_fileMap;
};

#endif //__HNSD_STORAGE_MANAGER_H__
