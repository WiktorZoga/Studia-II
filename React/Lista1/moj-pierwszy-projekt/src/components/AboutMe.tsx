interface AboutMeProps {
    description: string;
}

export const AboutMe = ({ description } : AboutMeProps) => {
    return (
        <div style={{ margin: '20px 0'}}>
            <h3> About me</h3>
            <p style={{ lineHeight: '1.6'}}>{description}</p>
        </div>
    );
};