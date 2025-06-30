// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
'use strict';

const { resolveModuleName } = require('ts-pnp');

exports.resolveModuleName = (
    typescript,
    moduleName,
    containingFile,
    compilerOptions,
    resolutionHost,
) => {
    return resolveModuleName(
        moduleName,
        containingFile,
        compilerOptions,
        resolutionHost,
        typescript.resolveModuleName,
    );
};

exports.resolveTypeReferenceDirective = (
    typescript,
    moduleName,
    containingFile,
    compilerOptions,
    resolutionHost,
) => {
    return resolveModuleName(
        moduleName,
        containingFile,
        compilerOptions,
        resolutionHost,
        typescript.resolveTypeReferenceDirective,
    );
};
