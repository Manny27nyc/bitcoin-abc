// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
import React from 'react';
import useWallet from '../hooks/useWallet';
export const WalletContext = React.createContext();

export const WalletProvider = ({ children }) => {
    const wallet = useWallet();
    return (
        <WalletContext.Provider value={wallet}>
            {children}
        </WalletContext.Provider>
    );
};
