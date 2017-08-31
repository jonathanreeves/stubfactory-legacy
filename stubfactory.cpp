/*!
 * MIT License
 *
 * Copyright (c) 2017 jonathanreeves
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <clang-c/Index.h>

#include <iostream>
#include <vector>

/*!
 * @brief TODO
 */
class FunctionInfo
{
public:
    FunctionInfo() {}
    ~FunctionInfo() {}

    const char *name;
    const char *retType;
    CXTypeKind retTypeKind;
    const char *className; // NULL for plain C functions
    std::vector<const char *> argNames;
    std::vector<const char *> argTypes;
};

std::vector<CXString> g_namespaces;

/*!
 * @brief TODO
 */
CXChildVisitResult functionDeclParamVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_ParmDecl){
        FunctionInfo *info = reinterpret_cast<FunctionInfo *>(client_data);

        CXType type = clang_getCursorType(cursor);
        CXString typeName = clang_getTypeSpelling(type);
        CXString paramName = clang_getCursorSpelling(cursor);
        
        info->argNames.push_back(clang_getCString(paramName));
        info->argTypes.push_back(clang_getCString(typeName));
    }

    return CXChildVisit_Continue;
    
}

/*!
 * @brief TODO
 */
void fillFunctionInfo(CXCursor cursor, FunctionInfo &info, const char *className)
{
    CXType funcType = clang_getCursorType(cursor);
    CXType returnType = clang_getResultType(funcType);
    CXString typeName = clang_getTypeSpelling(returnType);
    CXString cursorName = clang_getCursorSpelling(cursor);

    info.className = className;
    info.name = clang_getCString(cursorName);
    info.retType = clang_getCString(typeName);
    info.retTypeKind = returnType.kind; // needed for determining default

    // visit function children (looking for params)
    clang_visitChildren(cursor, *functionDeclParamVisitor, &info);
}

/*!
 * @brief TODO
 */
void printFunctionInfo(FunctionInfo &info)
{
    const char *funcPrefix = "file";

    if (info.className != NULL) {
        funcPrefix = info.className;
    }

    printf("%s ", info.retType);
    if (info.className != NULL) {
        printf("%s::", info.className);
    }
    printf("%s(", info.name);

    if (info.argTypes.size() > 0) {
        for (size_t i = 0; i < info.argTypes.size() - 1; i++) {
            printf("%s %s, ", info.argTypes[i], info.argNames[i]);
        }
        printf("%s %s", info.argTypes.back(), info.argNames.back());
    }
    printf(")\n{\n");

    if (info.retTypeKind != CXType_Void) {
        printf("    %s ret = g_%s_%s_return;\n", info.retType, funcPrefix, info.name);
    }
    printf("    g_%s_%s_callCount++;\n", funcPrefix, info.name);
    for (size_t i = 0; i < info.argTypes.size(); i++) {
        printf("    g_%s_%s_%s = %s;\n", funcPrefix, info.name, info.argNames[i], info.argNames[i]);
    }
    printf("    if (g_%s_%s_hook != NULL) {\n", funcPrefix, info.name);
    printf("        ");
    if (info.retTypeKind != CXType_Void) {
        printf("ret = ");
    }
    printf("g_%s_%s_hook(", funcPrefix, info.name);
    if (info.argNames.size() > 0) {
        for (size_t i = 0; i < info.argNames.size() - 1; i++) {
            printf("%s, ", info.argNames[i]);
        }
        printf("%s", info.argNames.back());
    }
    printf(");\n    } \n");
    if (info.retTypeKind != CXType_Void) {
        printf("    return ret;\n");
    }
    printf("}\n");
}

/*!
 * @brief TODO
 */
CXChildVisitResult classMethodVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    CXCursorKind kind = clang_getCursorKind(cursor);
    if(kind == CXCursorKind::CXCursor_CXXMethod) {
        CXString *className = reinterpret_cast<CXString *>(client_data);

        FunctionInfo info;
        fillFunctionInfo(cursor, info, clang_getCString(*className));
        printFunctionInfo(info);
    }
    return CXChildVisit_Continue;
}

/*!
 * @brief TODO
 */
CXChildVisitResult nodeVisitor(CXCursor cursor, CXCursor, CXClientData)
{
    CXChildVisitResult res = CXChildVisit_Continue;

    // skip all classes and functions not comming from the immediate file
    if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) != 0) {
        CXCursorKind kind = clang_getCursorKind(cursor);

        // Capture namespaces so they can be "used" with a using directive
        if (kind == CXCursorKind::CXCursor_Namespace) {
            CXString namespaceName = clang_getCursorSpelling(cursor);
            g_namespaces.push_back(namespaceName);
        }

        // Explore a class
        if (kind == CXCursorKind::CXCursor_ClassDecl) {
            CXString className = clang_getCursorSpelling(cursor);
            // visit function children (looking for methods)
            clang_visitChildren(cursor, *classMethodVisitor, &className);
        }

        // Explore "bare" C functions (outside of a class)
        if (kind == CXCursorKind::CXCursor_FunctionDecl) {
            FunctionInfo info;
            fillFunctionInfo(cursor, info, NULL);
            printFunctionInfo(info);
        }
        res = CXChildVisit_Recurse;
    }

    return res;
}

/*!
 * @brief TODO
 */
int main(int argc, char **argv)
{  
    CXErrorCode parseRes;
    CXTranslationUnit translationUnit;
    int retVal = 0;

    if (argc < 2) {
        retVal = -1;
        return retVal;
    }

    // Create an index with excludeDeclsFromPCH = 1, displayDiagnostics = 0
    CXIndex index = clang_createIndex(1, 0);

    // Skip function bodies, pass args as if clang were invoked directly
    // from the command line
    parseRes = clang_parseTranslationUnit2FullArgv(
        index,
        NULL,
        argv,
        argc,
        NULL,
        0,
        CXTranslationUnit_SkipFunctionBodies,
        &translationUnit);

    if (parseRes != CXError_Success) {
        fprintf(stderr, "ERROR: parser returned code %d\n", parseRes);
        retVal = -1;
        return retVal;
    }

    for (unsigned int i = 0; i < clang_getNumDiagnostics(translationUnit); i++) {
        CXDiagnostic diag = clang_getDiagnostic(translationUnit, i);
        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
        if (severity > CXDiagnostic_Warning) {
            CXString diagText = clang_getDiagnosticSpelling(diag);
            fprintf(stderr, "ERROR: %s\n", clang_getCString(diagText));
            clang_disposeString(diagText);
            retVal = -1;
        }
        clang_disposeDiagnostic(diag);
    }

    if (retVal != 0) {
        // don't continue
        return retVal;
    }

    // Visit all the nodes in the AST
    CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(cursor, nodeVisitor, 0);

    for (size_t i = 0; i < g_namespaces.size(); i++) {
        if (strcmp("", clang_getCString(g_namespaces[i])) != 0) {
            printf("using namespace %s;\n", clang_getCString(g_namespaces[i]));
        }
    }

    // Release memory
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    return 0;
}
