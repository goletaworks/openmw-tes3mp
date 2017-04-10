#ifndef OPENMW_ACTORAPI_HPP
#define OPENMW_ACTORAPI_HPP

#define ACTORAPI \
    {"InitScriptActorList",        ActorFunctions::InitScriptActorList},\
    \
    {"GetActorListSize",           ActorFunctions::GetActorListSize},\
    {"GetLastActorListAction",     ActorFunctions::GetLastActorListAction},\
    \
    {"GetActorRefId",              ActorFunctions::GetActorRefId},\
    {"GetActorRefNumIndex",        ActorFunctions::GetActorRefNumIndex},\
    {"GetActorMpNum",              ActorFunctions::GetActorMpNum},\
    \
    {"SetScriptActorListCell",     ActorFunctions::SetScriptActorListCell},\
    {"SetScriptActorListAction",   ActorFunctions::SetScriptActorListAction},\
    \
    {"SetActorRefId",              ActorFunctions::SetActorRefId},\
    {"SetActorRefNumIndex",        ActorFunctions::SetActorRefNumIndex},\
    {"SetActorMpNum",              ActorFunctions::SetActorMpNum},\
    \
    {"AddActor",                   ActorFunctions::AddActor},\
    \
    {"SendActorList",              ActorFunctions::SendActorList},\
    {"SendActorAuthority",         ActorFunctions::SendActorAuthority}

class ActorFunctions
{
public:

    static void InitScriptActorList(unsigned short pid) noexcept;

    static unsigned int GetActorListSize() noexcept;
    static unsigned char GetLastActorListAction() noexcept;

    static const char *GetActorRefId(unsigned int i) noexcept;
    static int GetActorRefNumIndex(unsigned int i) noexcept;
    static int GetActorMpNum(unsigned int i) noexcept;

    static void SetScriptActorListCell(const char* cellDescription) noexcept;
    static void SetScriptActorListAction(unsigned char action) noexcept;

    static void SetActorRefId(const char* refId) noexcept;
    static void SetActorRefNumIndex(int refNumIndex) noexcept;
    static void SetActorMpNum(int mpNum) noexcept;

    static void AddActor() noexcept;

    static void SendActorList() noexcept;
    static void SendActorAuthority() noexcept;
};


#endif //OPENMW_ACTORAPI_HPP