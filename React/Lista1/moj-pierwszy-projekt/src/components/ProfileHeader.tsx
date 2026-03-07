import { Avatar } from './Avatar';
import { Title } from './Title';

interface ProfileHeaderProps {
    name: string;
    role: string;
    avatarUrl: string;
}

export const ProfileHeader = ({ name, role, avatarUrl} : ProfileHeaderProps) => {
    return (
        <div style={{ textAlign: 'center', borderBottom: '2px solid #000000ff', paddingBottom: '20px' }}>
            <Avatar imageUrl={avatarUrl} />
            <Title name={name} role={role} />
        </div>
    );
};

