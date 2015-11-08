#include <cc_option.h>

#include <check.h>

#include <stdlib.h>
#include <stdio.h>

#define SUITE_NAME "option"
#define DEBUG_LOG  SUITE_NAME ".log"

#define TEST_OPTION(ACTION)                                                                              \
    ACTION( boolean,        OPTION_TYPE_BOOL,   "yes",          "it may be true of false"               )\
    ACTION( string,         OPTION_TYPE_STR,    "foo",          "it is a sequence of bytes"             )\
    ACTION( uinteger,       OPTION_TYPE_UINT,   "1",            "it is a non-negative integer number"   )

#define SETTING(ACTION)         \
    TEST_OPTION(ACTION)

struct setting {
    SETTING(OPTION_DECLARE)
};

static struct setting setting = {
    SETTING(OPTION_INIT)
};

static const unsigned int nopt = OPTION_CARDINALITY(struct setting);

/*
 * utilities
 */
static void
test_setup(void)
{
}

static void
test_teardown(void)
{
}

static void
test_reset(void)
{
    test_teardown();
    test_setup();
}


static char *
tmpname_create(char *data)
{
#define PATH "/tmp/temp.XXXXXX"
    size_t datalen = strlen(data);

    char *path = malloc(sizeof(PATH) + 3);
    strcpy(path, PATH);
    mkdtemp(path);
    size_t len = strlen(path);
    path[len++] = '/';
    path[len++] = '1';
    path[len++] = 0;

    FILE *fp = fopen(path, "w");
    ck_assert_uint_eq(fwrite(data, 1, datalen, fp), datalen);
    fclose(fp);

    return path;
#undef PATH
}

static void
tmpname_destroy(char *path)
{
    unlink(path);
    path[strlen(path) - 2] = 0;
    rmdir(path);
    free(path);
}

START_TEST(test_load_file)
{
    char *tmpname;
    FILE *fp;

    test_reset();

    tmpname = tmpname_create(
        "boolean: no\n"
        "string: bar\n"
        "uinteger: 3\n"
    );
    ck_assert_ptr_ne(tmpname, NULL);

    fp = fopen(tmpname, "r");
    ck_assert_ptr_ne(fp, NULL);

    ck_assert_int_eq(option_load_default((struct option *)&setting, nopt), CC_OK);
    ck_assert_int_eq(setting.boolean.val.vbool, true);
    ck_assert_int_eq(setting.uinteger.val.vuint, 1);
    ck_assert_str_eq(setting.string.val.vstr, "foo");

    ck_assert_int_eq(option_load_file(fp, (struct option *)&setting, nopt), CC_OK);
    ck_assert_int_eq(setting.boolean.val.vbool, false);
    ck_assert_int_eq(setting.uinteger.val.vuint, 3);
    ck_assert_str_eq(setting.string.val.vstr, "bar");

    option_free((struct option *)&setting, nopt);
    fclose(fp);

    tmpname_destroy(tmpname);
}
END_TEST

/*
 * test suite
 */
static Suite *
option_suite(void)
{
    Suite *s = suite_create(SUITE_NAME);

    TCase *tc_option = tcase_create("option test");
    tcase_add_test(tc_option, test_load_file);
    suite_add_tcase(s, tc_option);

    return s;
}
/**************
 * test cases *
 **************/

int
main(void)
{
    int nfail;

    /* setup */
    test_setup();

    Suite *suite = option_suite();
    SRunner *srunner = srunner_create(suite);
    srunner_set_log(srunner, DEBUG_LOG);
    srunner_run_all(srunner, CK_ENV); /* set CK_VEBOSITY in ENV to customize */
    nfail = srunner_ntests_failed(srunner);
    srunner_free(srunner);

    /* teardown */
    test_teardown();

    return (nfail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
