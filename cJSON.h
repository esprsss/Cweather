/*
  Авторские права (c) 2009-2017 Dave Gamble и контрибьюторы cJSON

  Данная лицензия разрешает лицам, получившим копию данного программного
  обеспечения и сопутствующей документации (далее — «Программное обеспечение»),
  безвозмездно использовать Программное обеспечение без ограничений, включая
  безусловное право на использование, копирование, изменение, объединение, публикацию,
  распространение, сублицензирование и/или продажу копий Программного обеспечения,
  а также лицам, которым предоставляется данное Программное обеспечение,
  при соблюдении следующих условий:

  Вышеуказанное уведомление об авторских правах и данное уведомление о разрешении
  должны быть включены во все копии или существенные части Программного обеспечения.

  ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ ПРЕДОСТАВЛЯЕТСЯ «КАК ЕСТЬ», БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ,
  ЯВНО ВЫРАЖЕННЫХ ИЛИ ПОДРАЗУМЕВАЕМЫХ, ВКЛЮЧАЯ, НО НЕ ОГРАНИЧИВАЯСЬ ГАРАНТИЯМИ
  ТОВАРНОЙ ПРИГОДНОСТИ, СООТВЕТСТВИЯ ОПРЕДЕЛЕННОМУ НАЗНАЧЕНИЮ И ОТСУТСТВИЯ НАРУШЕНИЙ.
  НИ В КОЕМ СЛУЧАЕ АВТОРЫ ИЛИ ПРАВООБЛАДАТЕЛИ НЕ НЕСУТ ОТВЕТСТВЕННОСТИ ПО ИСКАМ
  О ВОЗМЕЩЕНИИ УЩЕРБА ИЛИ ДРУГИМ ТРЕБОВАНИЯМ.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* При компиляции под Windows мы явно задаем соглашение о вызовах (calling convention),
чтобы избежать проблем при вызове из проектов с другим соглашением по умолчанию.
Для Windows доступны 3 директивы preprocessor:

CJSON_HIDE_SYMBOLS   - задается, когда вы вообще не хотите экспортировать символы (dllexport)
CJSON_EXPORT_SYMBOLS - задается при сборке библиотеки, если символы нужно экспортировать (по умолчанию)
CJSON_IMPORT_SYMBOLS - задается при подключении библиотеки, когда требуется dllimport

Для *nix-систем с поддержкой атрибута видимости (visibility) можно добиться аналогичного поведения:
установить скрытую видимость по умолчанию флагами:
-fvisibility=hidden (для gcc)
или
-xldscope=hidden (для sun cc)
в CFLAGS,

а затем флаг CJSON_API_VISIBILITY «экспортирует» те же символы аналогично CJSON_EXPORT_SYMBOLS.
*/

#define CJSON_CDECL __cdecl
#define CJSON_STDCALL __stdcall

/* Экспортируем символы по умолчанию — это необходимо для простой вставки исходников (.c и .h) в проект */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type)   type CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllexport) type CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllimport) type CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define CJSON_CDECL
#define CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* Версия проекта */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 19

#include <stddef.h>

/* Типы элементов cJSON: */
#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw    (1 << 7) /* Необработанный сырой JSON-текст */

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* Базовая структура cJSON: */
typedef struct cJSON
{
    /* Указатели next/prev позволяют обходить цепочки массива/объекта.
     * Либо используйте функции GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *next;
    struct cJSON *prev;
    /* У массива или объекта поле child указывает на цепочку вложенных элементов */
    struct cJSON *child;

    /* Тип элемента (значения из макросов выше) */
    int type;

    /* Строковое значение элемента, если type == cJSON_String или type == cJSON_Raw */
    char *valuestring;
    /* Прямая запись в valueint УСТАРЕЛА, вместо этого используйте cJSON_SetNumberValue */
    int valueint;
    /* Числовое значение элемента, если type == cJSON_Number */
    double valuedouble;

    /* Имя (ключ) элемента, если он находится внутри объекта */
    char *string;
} cJSON;

typedef struct cJSON_Hooks
{
      /* Функции malloc/free имеют соглашение CDECL на Windows независимо от настроек
       * компилятора по умолчанию, поэтому хуки должны принимать их напрямую */
      void *(CJSON_CDECL *malloc_fn)(size_t sz);
      void (CJSON_CDECL *free_fn)(void *ptr);
} cJSON_Hooks;

typedef int cJSON_bool;

/* Ограничение глубины вложенности массивов/объектов до отклонения парсинга.
 * Используется для предотвращения переполнения стека (stack overflow). */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* Ограничение длины циклических ссылок для защиты от переполнения стека. */
#ifndef CJSON_CIRCULAR_LIMIT
#define CJSON_CIRCULAR_LIMIT 10000
#endif

/* Возвращает версию библиотеки cJSON в виде строки */
CJSON_PUBLIC(const char*) cJSON_Version(void);

/* Передача кастомных функций выделения и освобождения памяти (malloc, free) в cJSON */
CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks);

/* Управление памятью: вызывающая сторона ВСЕГДА обязана освобождать результаты
 * всех вариантов cJSON_Parse (через cJSON_Delete) и cJSON_Print (через стандартный free,
 * cJSON_Hooks.free_fn или cJSON_free). Исключением является cJSON_PrintPreallocated,
 * где вызывающий код полностью берет на себя управление предоставленным буфером. */

/* Принимает блок JSON-текста и возвращает объект cJSON для чтения/анализа */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length);

/* ParseWithOpts позволяет потребовать завершение нулем (null-terminated)
 * и вернуть указатель на последний разобранный байт.
 * Если указан return_parse_end и произошла ошибка, он будет указывать на позицию ошибки (как cJSON_GetErrorPtr). */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);

/* Преобразует сущность cJSON в форматированный текст (для передачи или сохранения) */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item);
/* Преобразует сущность cJSON в компактный текст без форматирования (минифицированный) */
CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item);
/* Преобразует cJSON в текст с буферизацией. prebuffer — предварительная оценка размера буфера. fmt=0 без форматирования, fmt=1 с форматированием */
CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt);
/* Выводит cJSON в заранее выделенный буфер в памяти заданной длины. Возвращает 1 при успехе, 0 при ошибке. */
/* ВАЖНО: cJSON не всегда со 100% точностью оценивает объем памяти, поэтому для безопасности выделите на 5 байт больше, чем требуется */
CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format);
/* Удаляет сущность cJSON и все её дочерние ветви из памяти */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);

/* Возвращает количество элементов в массиве (или объекте) */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array);
/* Получить элемент по индексу из массива. Возвращает NULL, если не найден */
CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index);
/* Получить поле объекта по имени (без учета регистра) */
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string);
/* Получить поле объекта по имени (с учетом регистра) */
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string);
/* Проверяет наличие поля с заданным именем в объекте */
CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string);
/* Для анализа ошибок парсинга. Возвращает указатель на место ошибки. Работает, когда cJSON_Parse() возвращает 0 (NULL). При успехе равен 0. */
CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void);

/* Проверка типа элемента и получение его значения */
CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item);
CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item);

/* Функции проверки типа элемента */
CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item);

/* Функции создания узлов cJSON соответствующего типа */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean);
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num);
CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);
/* Создать необработанный кусок JSON */
CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw);
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void);

/* Создает строковый узел, ссылающийся на существующую строку (не будет освобождаться через cJSON_Delete) */
CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(const char *string);
/* Создает объект/массив, ссылающийся на элементы (они не будут освобождены через cJSON_Delete) */
CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(const cJSON *child);
CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(const cJSON *child);

/* Утилиты для создания массивов из обычных C-массивов с количеством элементов count.
 * Значение count не должно превышать реальный размер массива. */
CJSON_PUBLIC(cJSON *) cJSON_CreateIntArray(const int *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count);

/* Добавить элемент в указанный массив/объект */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
/* Используйте, когда строка ключа гарантированно константная (строковый литерал) и переживет объект cJSON.
 * ПРЕДУПРЕЖДЕНИЕ: При использовании этой функции перед записью в `item->string` убедитесь,
 * что флаг (item->type & cJSON_StringIsConst) равен нулю */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);
/* Добавить ссылку на элемент в массив/объект (когда нужно включить существующий узел без дублирования и порчи оригинала) */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);

/* Извлечение/отсоединение и удаление элементов из массивов и объектов */
CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);

/* Обновление элементов массива и объекта */
CJSON_PUBLIC(cJSON_bool) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem); /* Сдвигает существующие элементы вправо */
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * replacement);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object,const char *string,cJSON *newitem);

/* Создание полной глубокой копии (дублирование) узла cJSON */
CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse);
/* Функция Duplicate выделяет память под полную копию переданного узла.
 * При recurse!=0 рекурсивно копируются все дочерние узлы.
 * Указатели item->next и ->prev у нового узла всегда сброшены в 0. */

/* Рекурсивное сравнение двух элементов cJSON на идентичность.
 * Параметр case_sensitive определяет учет регистра ключей объектов (1 — учитывать, 0 — игнорировать) */
CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive);

/* Минификация строки: удаление пробельных символов (' ', '\t', '\r', '\n') из JSON-строки.
 * Указатель json не должен ссылаться на область памяти только для чтения (например, литерал),
 * а обязан указывать на перезаписываемый буфер. */
CJSON_PUBLIC(void) cJSON_Minify(char *json);

/* Вспомогательные функции для одновременного создания и добавления элементов в объект.
 * Возвращают указатель на добавленный узел или NULL при сбое. */
CJSON_PUBLIC(cJSON*) cJSON_AddNullToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddTrueToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddFalseToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean);
CJSON_PUBLIC(cJSON*) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number);
CJSON_PUBLIC(cJSON*) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string);
CJSON_PUBLIC(cJSON*) cJSON_AddRawToObject(cJSON * const object, const char * const name, const char * const raw);
CJSON_PUBLIC(cJSON*) cJSON_AddObjectToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddArrayToObject(cJSON * const object, const char * const name);

/* При установке целого числа оно также обязательно синхронизируется в valuedouble */
#define cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* Вспомогательная функция для макроса cJSON_SetNumberValue */
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number);
#define cJSON_SetNumberValue(object, number) ((object != NULL) ? cJSON_SetNumberHelper(object, (double)number) : (number))
/* Изменяет valuestring у узла типа cJSON_String (срабатывает, только если узел имеет тип cJSON_String) */
CJSON_PUBLIC(char*) cJSON_SetValuestring(cJSON *object, const char *valuestring);

/* Если объект не булевого типа, ничего не делает и возвращает cJSON_Invalid, иначе обновляет тип и возвращает его */
#define cJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (cJSON_False|cJSON_True))) ? \
    (object)->type=((object)->type &(~(cJSON_False|cJSON_True)))|((boolValue)?cJSON_True:cJSON_False) : \
    cJSON_Invalid\
)

/* Макрос для итерации (обхода циклом) по массиву или объекту */
#define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* Выделение и освобождение памяти через кастомные функции, заданные через cJSON_InitHooks */
CJSON_PUBLIC(void *) cJSON_malloc(size_t size);
CJSON_PUBLIC(void) cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
