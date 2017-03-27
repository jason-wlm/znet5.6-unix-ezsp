
#define CREATOR_CUSTOM 0xE700

#ifdef DEFINETYPES
typedef struct {
  int8u nodeType;
  int16u nodeId;
  int16u panId;
} tokTypeCustom;
#endif //DEFINETYPES

#ifdef DEFINETOKENS
DEFINE_BASIC_TOKEN(CUSTOM,
                   tokTypeCustom,
                   {0x00,0x0000,0x0000})
#endif //DEFINETOKENS
