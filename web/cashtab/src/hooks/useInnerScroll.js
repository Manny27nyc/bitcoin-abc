// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
import { useEffect } from 'react';

export const useInnerScroll = () =>
    useEffect(() => {
        document.body.style.overflow = 'hidden';
        return () => (document.body.style.overflow = '');
    }, []);
