import { ProfileHeader } from "./ProfileHeader";
import { ContactInfo } from "./ContactInfo";
import { AboutMe } from "./AboutMe";
import { SkillsList } from "./SkillsList";

interface BusinessCardProps {
    data: {
        name: string;
        role: string;
        avatarUrl: string
        email: string;
        phone: string;
        website: string;
        about: string;
        skills: string[]
    };
}

export const BussinessCard = ({ data } : BusinessCardProps) => {
    return (
        <div style={{
            maxWidth: '600px',
            margin: '40px auto',
            padding: '20px',
            boxShadow: '0 5px 8px rgba(0,0,0,0.1)',
            borderRadius: '15px',
            backgroundColor: '#739a7eff',
            fontFamily: 'sans-serif'
        }}>
            <ProfileHeader name={data.name} role={data.role} avatarUrl={data.avatarUrl} />
            <ContactInfo email={data.email} phone={data.phone} website={data.website} />
            <AboutMe description={data.about} />
            <SkillsList skills={data.skills} />
        </div>
    );
};