// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
export const checkForTokenById = (tokenList, specifiedTokenId) => {
    for (const t of tokenList) {
        if (t.tokenId === specifiedTokenId) {
            return true;
        }
    }
    return false;
};
