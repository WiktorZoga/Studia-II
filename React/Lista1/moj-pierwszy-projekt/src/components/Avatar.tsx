interface AvatarProps {
    imageUrl: string;
    altText?: string; 
}

export const Avatar = ({ imageUrl, altText = "Profile Picture" }: AvatarProps) => {
    return (
        <img src={imageUrl} alt={altText} style={{ width: '250px', borderRadius: '50%', border: '2px solid #000000ff' }} />
    );
};